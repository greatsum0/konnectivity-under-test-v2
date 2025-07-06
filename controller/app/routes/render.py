from __future__ import annotations

import io
from datetime import datetime, timezone
from pathlib import Path
from textwrap import indent
from typing import Dict, List

import yaml
from flask import Blueprint, jsonify, request
from jinja2 import StrictUndefined, Template

# ─────────────── blueprint & storage dir ────────────────
render_bp = Blueprint("render", __name__, url_prefix="/render")

MANIFEST_DIR = (Path(__file__).resolve().parent / "../../manifests").resolve()
MANIFEST_DIR.mkdir(parents=True, exist_ok=True)


# ─────────────── helpers ────────────────────────────────
def _read_text(fp: io.BytesIO | Path) -> str:  # bytes stream OR filesystem Path
    return fp.read().decode() if isinstance(fp, io.BytesIO) else Path(fp).read_text()


def _fill_defaults(scn: dict, defaults: dict, key: str, fallback: str) -> None:
    """Mirror the CLI’s default-filling helper."""
    sect = scn.setdefault(key, {})
    sect.setdefault("scenario", defaults.get(fallback, f"{key}.yaml"))


def _abort_missing(file: str, role: str) -> str:
    """Return a YAML comment marking the manifest invalid (kept for parity)."""
    return f"# ERROR: missing {role} config '{file}'\n"


# ─────────────── manifest builders ──────────────────────
def _render_servers(
    suite_yaml: dict, deploy_tpl: Template, cfg_files: Dict[str, str]
) -> str:
    suite_meta = suite_yaml.get("suite", {})
    defaults = suite_yaml.get("defaults", {})
    pvc_defaults = defaults.get("pvc", {})

    docs: List[str] = []

    for scn in suite_yaml.get("scenarios", []):
        _fill_defaults(scn, defaults, "server", "serverConfig")

        # ── ConfigMap ─────────────────────────────────────
        cm_name = f"kut-cm-deployment-server-{scn['name']}"
        cfg_file = scn["server"]["scenario"]
        if cfg_file not in cfg_files:
            return _abort_missing(cfg_file, "server")

        cfg_dict = yaml.safe_load(cfg_files[cfg_file])
        ep = (
            cfg_dict.setdefault("server", {})
            .setdefault("network", {})
            .setdefault("endpoint", {})
        )
        ep["ip"] = scn["server"].get("host", ep.get("ip", "0.0.0.0"))
        ep["port"] = scn["server"].get("port", ep.get("port"))
        ep["control_port"] = scn["server"].get("control_port", ep.get("control_port"))

        patched = yaml.safe_dump(cfg_dict, sort_keys=False)
        docs.append(
            (
                "apiVersion: v1\nkind: ConfigMap\nmetadata:\n"
                "  name: {name}\n  namespace: {ns}\n"
                "data:\n  {file}: |\n{body}"
            )
            .format(
                name=cm_name,
                ns=suite_meta.get("namespace", "default"),
                file=cfg_file,
                body=indent(patched.rstrip("\n"), "    "),
            )
            .strip()
        )

        # ── PVC ───────────────────────────────────────────
        pvc_name = f"kut-pvc-server-{scn['name']}"
        pvc_yaml = [
            "apiVersion: v1",
            "kind: PersistentVolumeClaim",
            "metadata:",
            f"  name: {pvc_name}",
            f"  namespace: {suite_meta.get('namespace', 'default')}",
            "spec:",
            "  accessModes: [ReadWriteOnce]",
            "  resources:",
            "    requests:",
            f"      storage: {pvc_defaults.get('storage', '1Gi')}",
        ]
        if pvc_defaults.get("storageClassName"):
            pvc_yaml.append(f"  storageClassName: {pvc_defaults['storageClassName']}")
        docs.append("\n".join(pvc_yaml).strip())

        # ── Deployment ────────────────────────────────────
        docs.append(
            deploy_tpl.render(suite=suite_meta, defaults=defaults, scenario=scn).strip()
        )

    return "\n---\n".join(docs) + "\n"


def _render_clients(
    suite_yaml: dict, job_tpl: Template, cfg_files: Dict[str, str]
) -> str:
    suite_meta = suite_yaml.get("suite", {})
    defaults = suite_yaml.get("defaults", {})
    pvc_defaults = defaults.get("pvc", {})

    docs: List[str] = []

    for scn in suite_yaml.get("scenarios", []):
        _fill_defaults(scn, defaults, "client", "scenarioFile")

        # ── ConfigMap ─────────────────────────────────────
        cm_name = f"kut-cm-job-client-{scn['name']}"
        cfg_file = scn["client"]["scenario"]
        if cfg_file not in cfg_files:
            return _abort_missing(cfg_file, "client")

        cfg_dict = yaml.safe_load(cfg_files[cfg_file])
        ep = (
            cfg_dict.setdefault("scenario", {})
            .setdefault("network", {})
            .setdefault("endpoint", {})
        )
        ep["ip"] = scn["client"].get("host", ep.get("ip", "0.0.0.0"))
        ep["port"] = scn["client"].get("port", ep.get("port"))
        ep["control_port"] = scn["client"].get("control_port", ep.get("control_port"))

        patched = yaml.safe_dump(cfg_dict, sort_keys=False)
        docs.append(
            (
                "apiVersion: v1\nkind: ConfigMap\nmetadata:\n"
                "  name: {name}\n  namespace: {ns}\n"
                "data:\n  {file}: |\n{body}"
            )
            .format(
                name=cm_name,
                ns=suite_meta.get("namespace", "default"),
                file=cfg_file,
                body=indent(patched.rstrip("\n"), "    "),
            )
            .strip()
        )

        # ── PVC ───────────────────────────────────────────
        pvc_name = f"kut-pvc-client-{scn['name']}"
        pvc_yaml = [
            "apiVersion: v1",
            "kind: PersistentVolumeClaim",
            "metadata:",
            f"  name: {pvc_name}",
            f"  namespace: {suite_meta.get('namespace', 'default')}",
            "spec:",
            "  accessModes: [ReadWriteOnce]",
            "  resources:",
            "    requests:",
            f"      storage: {pvc_defaults.get('storage', '1Gi')}",
        ]
        if pvc_defaults.get("storageClassName"):
            pvc_yaml.append(f"  storageClassName: {pvc_defaults['storageClassName']}")
        docs.append("\n".join(pvc_yaml).strip())

        # ── Job ───────────────────────────────────────────
        docs.append(
            job_tpl.render(suite=suite_meta, defaults=defaults, scenario=scn).strip()
        )

    return "\n---\n".join(docs) + "\n"


# ─────────────── HTTP endpoint ───────────────────────────
@render_bp.route("/suite", methods=["POST"])
def render_suite():
    """Render & persist manifests, returning only an acknowledgement JSON."""
    files = request.files
    for req_key in ("suite", "client_template", "server_template"):
        if req_key not in files:
            return jsonify({"error": f"Missing required part '{req_key}'"}), 400

    # read primary inputs
    suite_yaml = yaml.safe_load(files["suite"].read())
    job_tpl = Template(
        files["client_template"].read().decode(),
        undefined=StrictUndefined,
        trim_blocks=True,
        lstrip_blocks=True,
    )
    deploy_tpl = Template(
        files["server_template"].read().decode(),
        undefined=StrictUndefined,
        trim_blocks=True,
        lstrip_blocks=True,
    )

    cfg_files: Dict[str, str] = {}

    # allow either "config_dir" or "config_dir[]" as the form field name
    for field_name in ("config_dir", "config_dir[]"):
        for fp in request.files.getlist(field_name):   # ← grabs *all* uploads
            if fp and fp.filename:
                cfg_files[fp.filename] = fp.read().decode()

    # render
    server_yaml = _render_servers(suite_yaml, deploy_tpl, cfg_files)
    client_yaml = _render_clients(suite_yaml, job_tpl, cfg_files)

    # persist
    ts = datetime.now(timezone.utc).strftime("%Y%m%d%H%M%S")
    server_path = MANIFEST_DIR / f"server_out-{ts}.yaml"
    client_path = MANIFEST_DIR / f"client_out-{ts}.yaml"

    server_path.write_text(server_yaml)
    client_path.write_text(client_yaml)

    return (
        jsonify(
            {
                "message": "Manifests rendered and stored.",
                "server_file": str(server_path),
                "client_file": str(client_path),
            }
        ),
        201,
    )

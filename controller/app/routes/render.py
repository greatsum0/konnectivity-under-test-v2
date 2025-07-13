from __future__ import annotations

import io
from datetime import datetime, timezone
from pathlib import Path
from textwrap import indent
from typing import Dict, List

import yaml
from flask import Blueprint, jsonify, request
from jinja2 import StrictUndefined, Template

# ────────────── blueprint & storage dir ──────────────
render_bp = Blueprint("render", __name__, url_prefix="/render")

MANIFEST_DIR = (Path(__file__).resolve().parent / "../../manifests").resolve()
MANIFEST_DIR.mkdir(parents=True, exist_ok=True)


# ────────────── helpers ──────────────

def _read_text(fp: io.BytesIO | Path) -> str:
    return fp.read().decode() if isinstance(fp, io.BytesIO) else Path(fp).read_text()

def _fill_defaults(scn: dict, defaults: dict, key: str, fallback: str) -> None:
    sect = scn.setdefault(key, {})
    sect.setdefault("scenario", defaults.get(fallback, f"{key}.yaml"))

def _abort_missing(file: str, role: str) -> str:
    return f"# ERROR: missing {role} config '{file}'\n"


# ────────────── manifest builders ──────────────

def _render_servers(suite_yaml: dict, deploy_tpl: Template, cfg_files: Dict[str, str]) -> str:
    suite_meta = suite_yaml.get("suite", {})
    defaults = suite_yaml.get("defaults", {})
    pvc_defaults = defaults.get("pvc", {})
    docs: List[str] = []

    for scn in suite_yaml.get("scenarios", []):
        _fill_defaults(scn, defaults, "server", "serverConfig")
        cm_name = f"kut-cm-deployment-server-{scn['name']}"
        cfg_file = scn["server"]["scenario"]
        if cfg_file not in cfg_files:
            return _abort_missing(cfg_file, "server")

        cfg_dict = yaml.safe_load(cfg_files[cfg_file])
        ep = cfg_dict.setdefault("server", {}).setdefault("network", {}).setdefault("endpoint", {})
        ep["ip"] = scn["server"].get("host", ep.get("ip", "0.0.0.0"))
        ep["port"] = scn["server"].get("port", ep.get("port"))
        ep["control_port"] = scn["server"].get("control_port", ep.get("control_port"))

        patched = yaml.safe_dump(cfg_dict, sort_keys=False)
        docs.append(("""apiVersion: v1
kind: ConfigMap
metadata:
  name: {name}
  namespace: {ns}
data:
  {file}: |
{body}""").format(
            name=cm_name,
            ns=suite_meta.get("namespace", "default"),
            file=cfg_file,
            body=indent(patched.rstrip("\n"), "    "),
        ).strip())

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
            f"      storage: {pvc_defaults.get('storage', '1Gi')}"
        ]
        if pvc_defaults.get("storageClassName"):
            pvc_yaml.append(f"  storageClassName: {pvc_defaults['storageClassName']}")
        docs.append("\n".join(pvc_yaml).strip())

        docs.append(deploy_tpl.render(suite=suite_meta, defaults=defaults, scenario=scn).strip())

    return "\n---\n".join(docs) + "\n"

def _render_clients(suite_yaml: dict, job_tpl: Template, cfg_files: Dict[str, str]) -> str:
    suite_meta = suite_yaml.get("suite", {})
    defaults = suite_yaml.get("defaults", {})
    pvc_defaults = defaults.get("pvc", {})
    docs: List[str] = []

    for scn in suite_yaml.get("scenarios", []):
        _fill_defaults(scn, defaults, "client", "scenarioFile")
        cm_name = f"kut-cm-job-client-{scn['name']}"
        cfg_file = scn["client"]["scenario"]
        if cfg_file not in cfg_files:
            return _abort_missing(cfg_file, "client")

        cfg_dict = yaml.safe_load(cfg_files[cfg_file])
        ep = cfg_dict.setdefault("scenario", {}).setdefault("network", {}).setdefault("endpoint", {})
        ep["ip"] = scn["client"].get("host", ep.get("ip", "0.0.0.0"))
        ep["port"] = scn["client"].get("port", ep.get("port"))
        ep["control_port"] = scn["client"].get("control_port", ep.get("control_port"))

        patched = yaml.safe_dump(cfg_dict, sort_keys=False)
        docs.append(("""apiVersion: v1
kind: ConfigMap
metadata:
  name: {name}
  namespace: {ns}
data:
  {file}: |
{body}""").format(
            name=cm_name,
            ns=suite_meta.get("namespace", "default"),
            file=cfg_file,
            body=indent(patched.rstrip("\n"), "    "),
        ).strip())

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
            f"      storage: {pvc_defaults.get('storage', '1Gi')}"
        ]
        if pvc_defaults.get("storageClassName"):
            pvc_yaml.append(f"  storageClassName: {pvc_defaults['storageClassName']}")
        docs.append("\n".join(pvc_yaml).strip())

        docs.append(job_tpl.render(suite=suite_meta, defaults=defaults, scenario=scn).strip())

    return "\n---\n".join(docs) + "\n"

def _render_host_viewers(suite_yaml: dict) -> str:
    suite_meta = suite_yaml.get("suite", {})
    namespace = suite_meta.get("namespace", "default")
    host_to_pvcs: Dict[str, List[Dict[str, str]]] = {}

    for scn in suite_yaml.get("scenarios", []):
        name = scn["name"]
        if "host" in scn.get("server", {}):
            host = scn["server"]["host"]
            host_to_pvcs.setdefault(host, []).append({
                "pvc": f"kut-pvc-server-{name}",
                "mountPath": f"/mnt/{name}/server"
            })
        if "host" in scn.get("client", {}):
            host = scn["client"]["host"]
            host_to_pvcs.setdefault(host, []).append({
                "pvc": f"kut-pvc-client-{name}",
                "mountPath": f"/mnt/{name}/client"
            })

    docs: List[str] = []
    for host, mounts in host_to_pvcs.items():
        vol_lines, mount_lines = [], []
        for idx, mnt in enumerate(mounts):
            vol_name = f"vol{idx}"
            mount_lines.extend([
                f"        - name: {vol_name}",
                f"          mountPath: {mnt['mountPath']}",
                "          readOnly: true"
            ])
            vol_lines.extend([
                f"    - name: {vol_name}",
                "      persistentVolumeClaim:",
                f"        claimName: {mnt['pvc']}"
            ])

        pod_yaml = [
                       "apiVersion: v1",
                       "kind: Pod",
                       "metadata:",
                       f"  name: viewer-{host.replace('.', '-')}",
                       f"  namespace: {namespace}",
                       "  labels:",
                       "    app: viewer",
                       "spec:",
                       f"  nodeSelector:",
                       f"    kubernetes.io/hostname: {host}",
                       "  restartPolicy: Never",
                       "  containers:",
                       "    - name: viewer",
                       "      image: busybox",
                       "      command: ['sleep', '3600']",
                       "      volumeMounts:"
                   ] + mount_lines + ["  volumes:"] + vol_lines

        docs.append("\n".join(pod_yaml))

    return "\n---\n".join(docs) + "\n"


@render_bp.route("/suite", methods=["POST"])
def render_suite():
    files = request.files
    for req_key in ("suite", "client_template", "server_template"):
        if req_key not in files:
            return jsonify({"error": f"Missing required part '{req_key}'"}), 400

    suite_yaml = yaml.safe_load(files["suite"].read())
    job_tpl = Template(files["client_template"].read().decode(), undefined=StrictUndefined, trim_blocks=True, lstrip_blocks=True)
    deploy_tpl = Template(files["server_template"].read().decode(), undefined=StrictUndefined, trim_blocks=True, lstrip_blocks=True)

    cfg_files: Dict[str, str] = {}
    for field_name in ("config_dir", "config_dir[]"):
        for fp in request.files.getlist(field_name):
            if fp and fp.filename:
                cfg_files[fp.filename] = fp.read().decode()

    server_yaml = _render_servers(suite_yaml, deploy_tpl, cfg_files)
    client_yaml = _render_clients(suite_yaml, job_tpl, cfg_files)
    viewer_yaml = _render_host_viewers(suite_yaml)

    MANIFEST_DIR.mkdir(parents=True, exist_ok=True)

    ts = datetime.now(timezone.utc).strftime("%Y%m%d%H%M%S")
    server_path = MANIFEST_DIR / f"server_out-{ts}.yaml"
    client_path = MANIFEST_DIR / f"client_out-{ts}.yaml"
    viewer_path = MANIFEST_DIR / f"viewer_out-{ts}.yaml"

    server_path.write_text(server_yaml)
    client_path.write_text(client_yaml)
    viewer_path.write_text(viewer_yaml)

    return jsonify({
        "message": "Manifests rendered and stored.",
        "server_file": str(server_path),
        "client_file": str(client_path),
        "viewer_file": str(viewer_path)
    }), 201

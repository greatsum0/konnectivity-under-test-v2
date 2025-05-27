from flask import Blueprint, request, jsonify
import subprocess
from pathlib import Path

manifests_bp = Blueprint("manifests", __name__)

BASE_DIR = Path(__file__).resolve().parent.parent.parent
CONFIGMAP_DIR = BASE_DIR / "configmaps"
JOBS_DIR = BASE_DIR / "jobs"
VOLUME_DIR = BASE_DIR / "volumes"


@manifests_bp.route("/manifests/apply", methods=["POST"])
def apply_manifests():
    data = request.get_json()
    m_type = data.get("type", "all")
    files = data.get("files")  # opsiyonel
    namespace = data.get("namespace")

    if not namespace:
        return jsonify({"error": "Missing required field: 'namespace'"}), 400

    results = []

    if m_type == "all":
        results.extend(apply_all_yamls(CONFIGMAP_DIR, "configmaps", namespace))
        results.extend(apply_all_yamls(JOBS_DIR, "jobs", namespace))
    elif m_type in ["configmaps", "jobs"]:
        if not files or not isinstance(files, list):
            return jsonify({"error": "Missing or invalid 'files' list."}), 400
        target_dir = CONFIGMAP_DIR if m_type == "configmaps" else JOBS_DIR
        results.extend(apply_selected_files(target_dir, files, m_type, namespace))
    if m_type in ["volumes", "all"]:
        results.extend(apply_all_yamls(VOLUME_DIR, "volumes", namespace))
    else:
        return (
            jsonify({"error": "Invalid type. Must be 'all', 'jobs' or 'configmaps'."}),
            400,
        )

    return jsonify({"applied": results}), 200


def apply_all_yamls(directory: Path, label: str, namespace: str):
    if not directory.exists():
        return [{"type": label, "status": "skipped", "reason": "Directory not found"}]

    files = [f.name for f in directory.glob("*.yaml")]
    return apply_selected_files(directory, files, label, namespace)


def apply_selected_files(directory: Path, file_list: list, label: str, namespace: str):
    applied = []
    for fname in file_list:
        path = directory / fname
        if not path.exists():
            applied.append({"type": label, "file": fname, "status": "not_found"})
            continue
        try:
            subprocess.run(
                ["kubectl", "apply", "-f", str(path), "-n", namespace],
                check=True,
                capture_output=True,
                text=True,
            )
            applied.append({"type": label, "file": fname, "status": "applied"})
        except subprocess.CalledProcessError as e:
            applied.append(
                {"type": label, "file": fname, "status": "failed", "error": e.stderr}
            )
    return applied

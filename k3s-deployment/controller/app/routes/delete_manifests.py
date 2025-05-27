from flask import Blueprint, request, jsonify
import subprocess
from pathlib import Path

delete_manifests_bp = Blueprint("delete_manifests", __name__)

BASE_DIR = Path(__file__).resolve().parent.parent.parent
CONFIGMAP_DIR = BASE_DIR / "configmaps"
JOBS_DIR = BASE_DIR / "jobs"

@delete_manifests_bp.route("/manifests/delete", methods=["DELETE"])
def delete_manifests():
    data = request.get_json()
    m_type = data.get("type")
    namespace = data.get("namespace")
    files = data.get("files")  # opsiyonel, type != all ise zorunlu

    if not namespace:
        return jsonify({"error": "Missing 'namespace'"}), 400

    if m_type == "all":
        results = []
        results.extend(delete_all_yaml(CONFIGMAP_DIR, "configmaps", namespace))
        results.extend(delete_all_yaml(JOBS_DIR, "jobs", namespace))
        return jsonify({"deleted": results}), 200

    if m_type not in ["jobs", "configmaps"]:
        return jsonify({"error": "Invalid type. Must be 'jobs', 'configmaps' or 'all'."}), 400
    if not files or not isinstance(files, list):
        return jsonify({"error": "Missing or invalid 'files' list."}), 400

    target_dir = JOBS_DIR if m_type == "jobs" else CONFIGMAP_DIR
    results = delete_selected_files(target_dir, files, namespace)
    return jsonify({"deleted": results}), 200


def delete_selected_files(directory: Path, file_list: list, namespace: str):
    results = []
    for fname in file_list:
        manifest_path = directory / fname
        if not manifest_path.exists():
            results.append({"file": fname, "status": "not_found"})
            continue
        try:
            subprocess.run(
                ["kubectl", "delete", "-f", str(manifest_path), "-n", namespace],
                check=True,
                capture_output=True,
                text=True
            )
            results.append({"file": fname, "status": "deleted"})
        except subprocess.CalledProcessError as e:
            results.append({"file": fname, "status": "failed", "error": e.stderr.strip()})
    return results


def delete_all_yaml(directory: Path, label: str, namespace: str):
    if not directory.exists():
        return [{"type": label, "status": "skipped", "reason": "Directory not found"}]

    all_files = [f.name for f in directory.glob("*.yaml")]
    return delete_selected_files(directory, all_files, namespace)

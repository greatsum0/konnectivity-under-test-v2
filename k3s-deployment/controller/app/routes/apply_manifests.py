# Flask Blueprints for configmap, job, pvc apply endpoints
from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException
from pathlib import Path
import yaml

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()
batch_v1 = client.BatchV1Api()

BASE_DIR = Path(__file__).resolve().parent.parent.parent
CONFIGMAP_DIR = BASE_DIR / "configmaps"
JOBS_DIR = BASE_DIR / "jobs"
VOLUME_DIR = BASE_DIR / "volumes"

apply_configmap_bp = Blueprint("apply_configmap", __name__)
apply_job_bp = Blueprint("apply_job", __name__)
apply_pvc_bp = Blueprint("apply_pvc", __name__)


# ---------------- ConfigMap -------------------
@apply_configmap_bp.route("/apply/configmap", methods=["POST"])
def apply_configmap():
    return apply_resource_batch(
        CONFIGMAP_DIR,
        core_v1.read_namespaced_config_map,
        core_v1.replace_namespaced_config_map,
        core_v1.create_namespaced_config_map,
        "ConfigMap",
    )


# ---------------- Job -------------------------
@apply_job_bp.route("/apply/job", methods=["POST"])
def apply_job():
    return apply_resource_batch(
        JOBS_DIR,
        batch_v1.read_namespaced_job,
        batch_v1.replace_namespaced_job,
        batch_v1.create_namespaced_job,
        "Job",
    )


# ---------------- PVC -------------------------
@apply_pvc_bp.route("/apply/pvc", methods=["POST"])
def apply_pvc():
    return apply_resource_batch(
        VOLUME_DIR,
        core_v1.read_namespaced_persistent_volume_claim,
        None,  # PVC is immutable, skip replace
        core_v1.create_namespaced_persistent_volume_claim,
        "PersistentVolumeClaim",
    )


# ---------------- Common Handler --------------
def apply_resource_batch(directory: Path, read_fn, replace_fn, create_fn, kind: str):
    data = request.get_json()
    namespace = data.get("namespace")
    files = data.get("files")

    if not namespace:
        return jsonify({"error": "Missing 'namespace' field"}), 400
    if not files or not isinstance(files, list):
        return jsonify({"error": "Missing or invalid 'files' list"}), 400

    if "*" in files:
        files = [f.name for f in directory.glob("*.yaml")]

    results = []
    for file_name in sorted(set(files)):
        manifest_path = directory / file_name
        if not manifest_path.exists():
            results.append({"file": file_name, "status": "not_found"})
            continue

        try:
            with open(manifest_path) as f:
                doc = yaml.safe_load(f)
                if not doc:
                    raise ValueError("Empty or invalid YAML")

                if "metadata" not in doc:
                    doc["metadata"] = {}
                doc["metadata"]["namespace"] = namespace
                name = doc["metadata"].get("name")

                try:
                    if kind == "PersistentVolumeClaim":
                        read_fn(name, namespace)
                        results.append(
                            {
                                "file": file_name,
                                "kind": kind,
                                "name": name,
                                "status": "skipped",
                                "reason": "PVC is immutable and exists",
                            }
                        )
                        continue
                    read_fn(name, namespace)
                    replace_fn(name, namespace, doc)
                    results.append(
                        {
                            "file": file_name,
                            "kind": kind,
                            "name": name,
                            "action": "replaced",
                        }
                    )
                except ApiException as e:
                    if e.status == 404:
                        create_fn(namespace, doc)
                        results.append(
                            {
                                "file": file_name,
                                "kind": kind,
                                "name": name,
                                "action": "created",
                            }
                        )
                    else:
                        raise e

        except (ApiException, Exception) as e:
            results.append(
                {"file": file_name, "kind": kind, "status": "failed", "error": str(e)}
            )

    return jsonify({"applied": results}), 200

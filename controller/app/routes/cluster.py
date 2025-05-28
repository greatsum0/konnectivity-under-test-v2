from flask import Blueprint, request, jsonify
from kubernetes import client
from kubernetes.client.rest import ApiException
from app.utils.kube_utils import load_kube_config

# Load Kubernetes configuration (from kubeconfig or in-cluster)
load_kube_config()

# Kubernetes API clients
core_v1 = client.CoreV1Api()
apps_v1 = client.AppsV1Api()
batch_v1 = client.BatchV1Api()
events_v1 = client.EventsV1Api()
networking_v1 = client.NetworkingV1Api()

cluster_bp = Blueprint("cluster", __name__)


@cluster_bp.route("/cluster/reset", methods=["DELETE"])
def reset_namespace():
    """
    Deletes the given namespace if 'confirm' is True.

    JSON payload:
    {
        "namespace": "example-ns",
        "confirm": true
    }
    """
    data = request.get_json()
    namespace = data.get("namespace")
    confirm = data.get("confirm")

    if not namespace or confirm is not True:
        return jsonify({"error": "Missing 'namespace' or 'confirm' must be true"}), 400

    try:
        core_v1.delete_namespace(name=namespace)
        return (
            jsonify({"status": "namespace deletion initiated", "namespace": namespace}),
            202,
        )

    except ApiException as e:
        return jsonify({"error": "Kubernetes API error", "details": e.body}), 500
    except Exception as e:
        return jsonify({"error": "Internal server error", "details": str(e)}), 500


@cluster_bp.route("/cluster/status", methods=["GET"])
def cluster_status():
    """
    Returns a resource summary for a given namespace.

    Query parameter:
    - namespace: name of the namespace
    """
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    result = {
        "namespace": namespace,
        "nodes": [],
        "deployments": [],
        "jobs": [],
        "pods": [],
        "pvcs": [],
        "pvs": [],
        "secrets": [],
        "configmaps": [],
        "services": [],
        "ingresses": [],
        "events": [],
    }

    def safe_list(getter, transform):
        try:
            return transform(getter())
        except ApiException:
            return []

    result["nodes"] = safe_list(
        lambda: core_v1.list_node().items,
        lambda items: [n.metadata.name for n in items],
    )

    result["deployments"] = safe_list(
        lambda: apps_v1.list_namespaced_deployment(namespace).items,
        lambda items: [d.metadata.name for d in items],
    )

    result["jobs"] = safe_list(
        lambda: batch_v1.list_namespaced_job(namespace).items,
        lambda items: [
            {
                "name": j.metadata.name,
                "status": (
                    "Running"
                    if j.status.active
                    else "Completed" if j.status.succeeded else "Unknown"
                ),
            }
            for j in items
        ],
    )

    result["pods"] = safe_list(
        lambda: core_v1.list_namespaced_pod(namespace).items,
        lambda items: [
            {"name": p.metadata.name, "phase": p.status.phase} for p in items
        ],
    )

    result["pvcs"] = safe_list(
        lambda: core_v1.list_namespaced_persistent_volume_claim(namespace).items,
        lambda items: [p.metadata.name for p in items],
    )

    result["pvs"] = safe_list(
        lambda: core_v1.list_persistent_volume().items,
        lambda items: [p.metadata.name for p in items],
    )

    result["secrets"] = safe_list(
        lambda: core_v1.list_namespaced_secret(namespace).items,
        lambda items: [s.metadata.name for s in items],
    )

    result["configmaps"] = safe_list(
        lambda: core_v1.list_namespaced_config_map(namespace).items,
        lambda items: [c.metadata.name for c in items],
    )

    result["services"] = safe_list(
        lambda: core_v1.list_namespaced_service(namespace).items,
        lambda items: [s.metadata.name for s in items],
    )

    result["ingresses"] = safe_list(
        lambda: networking_v1.list_namespaced_ingress(namespace).items,
        lambda items: [i.metadata.name for i in items],
    )

    result["events"] = safe_list(
        lambda: core_v1.list_namespaced_event(namespace, limit=10).items,
        lambda items: [
            {
                "timestamp": e.last_timestamp.isoformat() if e.last_timestamp else None,
                "involved_object": f"{e.involved_object.kind}/{e.involved_object.name}",
                "message": e.message,
            }
            for e in items
        ],
    )

    return jsonify(result), 200

from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException

cluster_bp = Blueprint("cluster", __name__)


@cluster_bp.route("/cluster/reset", methods=["DELETE"])
def reset_namespace():
    data = request.get_json()
    namespace = data.get("namespace")
    confirm = data.get("confirm")

    if not namespace or confirm != True:
        return (
            jsonify(
                {"error": "Missing 'namespace' or 'confirm' must be true to proceed"}
            ),
            400,
        )

    try:
        config.load_kube_config()
        v1 = client.CoreV1Api()

        v1.delete_namespace(name=namespace)

        return (
            jsonify({"status": "namespace deletion initiated", "namespace": namespace}),
            202,
        )

    except ApiException as e:
        return jsonify({"error": f"Kubernetes API error", "details": e.body}), 500
    except Exception as e:
        return jsonify({"error": str(e)}), 500


from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException
from datetime import datetime

cluster_bp = Blueprint("cluster", __name__)

# Load Kubernetes config
try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()
apps_v1 = client.AppsV1Api()
batch_v1 = client.BatchV1Api()
events_v1 = client.EventsV1Api()


@cluster_bp.route("/cluster/status", methods=["GET"])
def cluster_status():
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

    try:
        result["nodes"] = [n.metadata.name for n in core_v1.list_node().items]
    except:
        pass

    try:
        deployments = apps_v1.list_namespaced_deployment(namespace)
        result["deployments"] = [d.metadata.name for d in deployments.items]
    except:
        pass

    try:
        jobs = batch_v1.list_namespaced_job(namespace)
        result["jobs"] = [
            {
                "name": j.metadata.name,
                "status": (
                    j.status.active and "Running" or "Completed"
                    if j.status.succeeded
                    else "Unknown"
                ),
            }
            for j in jobs.items
        ]
    except:
        pass

    try:
        pods = core_v1.list_namespaced_pod(namespace)
        result["pods"] = [
            {"name": p.metadata.name, "phase": p.status.phase} for p in pods.items
        ]
    except:
        pass

    try:
        result["pvcs"] = [
            p.metadata.name
            for p in core_v1.list_namespaced_persistent_volume_claim(namespace).items
        ]
    except:
        pass

    try:
        result["pvs"] = [
            p.metadata.name for p in core_v1.list_persistent_volume().items
        ]
    except:
        pass

    try:
        result["secrets"] = [
            s.metadata.name for s in core_v1.list_namespaced_secret(namespace).items
        ]
    except:
        pass

    try:
        result["configmaps"] = [
            c.metadata.name for c in core_v1.list_namespaced_config_map(namespace).items
        ]
    except:
        pass

    try:
        result["services"] = [
            s.metadata.name for s in core_v1.list_namespaced_service(namespace).items
        ]
    except:
        pass

    try:
        networking_v1 = client.NetworkingV1Api()
        result["ingresses"] = [
            i.metadata.name
            for i in networking_v1.list_namespaced_ingress(namespace).items
        ]
    except:
        pass

    try:
        events = core_v1.list_namespaced_event(namespace, limit=10)
        result["events"] = [
            {
                "timestamp": e.last_timestamp.isoformat() if e.last_timestamp else None,
                "involved_object": f"{e.involved_object.kind}/{e.involved_object.name}",
                "message": e.message,
            }
            for e in events.items
        ]
    except:
        pass

    return jsonify(result), 200

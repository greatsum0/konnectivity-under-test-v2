from flask import Blueprint, jsonify, request
from kubernetes import client, config



pods_bp = Blueprint("pods", __name__)


@pods_bp.route("/pods", methods=["GET"])
def get_pods():
    namespace = request.args.get("namespace")  # None means "all"
    try:
        pods = list_pods(namespace)
        return jsonify(pods), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500


def list_pods(namespace: str = None):
    config.load_kube_config()  # Use load_incluster_config() inside cluster
    v1 = client.CoreV1Api()

    if namespace:
        pods = v1.list_namespaced_pod(namespace=namespace, watch=False)
    else:
        pods = v1.list_pod_for_all_namespaces(watch=False)

    return [
        {
            "name": pod.metadata.name,
            "namespace": pod.metadata.namespace,
            "status": pod.status.phase,
            "node": pod.spec.node_name,
        }
        for pod in pods.items
    ]

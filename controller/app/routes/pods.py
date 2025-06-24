from flask import Blueprint, jsonify, request
from kubernetes import client
from kubernetes.client.rest import ApiException
from app.utils.kube_utils import load_kube_config

# Blueprint definition
pods_bp = Blueprint("pods", __name__)

# Load Kubernetes configuration and client
load_kube_config()
core_v1 = client.CoreV1Api()


def list_pods(namespace: str = None):
    """
    List pods in a given namespace or across all namespaces.

    Args:
        namespace (str): Namespace to filter by (optional)

    Returns:
        list: List of pod metadata dictionaries

    Raises:
        RuntimeError: If Kubernetes API call fails
    """
    try:
        if namespace:
            pod_list = core_v1.list_namespaced_pod(namespace=namespace)
        else:
            pod_list = core_v1.list_pod_for_all_namespaces()

        return [
            {
                "name": pod.metadata.name,
                "namespace": pod.metadata.namespace,
                "status": pod.status.phase,
                "node": pod.spec.node_name,
            }
            for pod in pod_list.items
        ]

    except ApiException as e:
        raise RuntimeError(f"Kubernetes API error: {e.reason}") from e


@pods_bp.route("/pods", methods=["GET"])
def get_pods():
    """
    GET /pods?namespace=<namespace>
    Lists all pods in the specified namespace or across all if omitted.
    """
    namespace = request.args.get("namespace")

    try:
        pods = list_pods(namespace)
        return jsonify(pods), 200

    except RuntimeError as e:
        return jsonify({"error": str(e)}), 500

    except Exception as e:
        return jsonify({"error": "Internal server error", "details": str(e)}), 500


@pods_bp.route("/pods/log/<pod_name>", methods=["GET"])
def get_pod_logs(pod_name):
    """
    GET /pods/log/<pod_name>?namespace=<namespace>
    Retrieves the logs of the specified pod.

    Path Parameters:
        - pod_name (str): The name of the pod

    Query Parameters:
        - namespace (str): The namespace where the pod is located

    Returns:
        - Pod logs in JSON format
    """
    namespace = request.args.get("namespace")

    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        logs = core_v1.read_namespaced_pod_log(name=pod_name, namespace=namespace)
        return jsonify({"pod": pod_name, "namespace": namespace, "logs": logs}), 200

    except ApiException as e:
        return (
            jsonify({"error": f"Kubernetes API error: {e.reason}", "details": e.body}),
            e.status,
        )

    except Exception as e:
        return jsonify({"error": "Internal server error", "details": str(e)}), 500

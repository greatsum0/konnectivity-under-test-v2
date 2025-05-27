from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException

logs_bp = Blueprint("logs", __name__)

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()


@logs_bp.route("/logs/<pod_name>", methods=["GET"])
def get_pod_logs(pod_name):
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
        return jsonify({"error": str(e)}), 500

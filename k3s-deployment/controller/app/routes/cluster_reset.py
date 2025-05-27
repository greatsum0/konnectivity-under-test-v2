from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException

cluster_reset_bp = Blueprint("cluster_reset", __name__)

@cluster_reset_bp.route("/cluster/reset", methods=["DELETE"])
def reset_namespace():
    data = request.get_json()
    namespace = data.get("namespace")
    confirm = data.get("confirm")

    if not namespace or confirm != True:
        return jsonify({
            "error": "Missing 'namespace' or 'confirm' must be true to proceed"
        }), 400

    try:
        config.load_kube_config()
        v1 = client.CoreV1Api()

        v1.delete_namespace(name=namespace)

        return jsonify({
            "status": "namespace deletion initiated",
            "namespace": namespace
        }), 202

    except ApiException as e:
        return jsonify({
            "error": f"Kubernetes API error",
            "details": e.body
        }), 500
    except Exception as e:
        return jsonify({
            "error": str(e)
        }), 500

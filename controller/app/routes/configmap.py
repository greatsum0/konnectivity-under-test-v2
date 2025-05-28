from flask import Blueprint, request, jsonify
from app.services.configmap_generator import generate_configmaps_from_scenarios
from kubernetes import client, config
from kubernetes.client.rest import ApiException

configmap_bp = Blueprint("configmap", __name__)

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()

configmap_bp = Blueprint("configmap", __name__)


@configmap_bp.route("/configmaps/create", methods=["POST"])
def generate_configmaps():
    try:
        data = request.get_json()
        scenarios = data.get("scenarios")
        namespace = data.get("namespace", "default")

        if not scenarios or not isinstance(scenarios, list):
            return jsonify({"error": "Missing or invalid 'scenarios' list"}), 400

        result = generate_configmaps_from_scenarios(scenarios, namespace)
        return jsonify(result), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500


@configmap_bp.route("/configmaps", methods=["GET"])
def list_configmaps():
    namespace = request.args.get("namespace", "default")
    results = []

    try:
        configmaps = core_v1.list_namespaced_config_map(namespace=namespace)
        for cm in configmaps.items:
            results.append(
                {
                    "name": cm.metadata.name,
                    "namespace": cm.metadata.namespace,
                    "created_at": cm.metadata.creation_timestamp.isoformat(),
                }
            )

        return jsonify({"configmaps": results}), 200

    except ApiException as e:
        return jsonify({"error": "Kubernetes API error", "details": e.body}), 500
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@configmap_bp.route("/configmaps/<string:name>", methods=["DELETE"])
def delete_configmap(name):
    namespace = request.args.get("namespace", "default")

    try:
        core_v1.delete_namespaced_config_map(name=name, namespace=namespace)
        return jsonify({"status": "deleted", "name": name, "namespace": namespace}), 200

    except ApiException as e:
        if e.status == 404:
            return (
                jsonify(
                    {
                        "error": f"ConfigMap '{name}' not found in namespace '{namespace}'"
                    }
                ),
                404,
            )
        return jsonify({"error": "Kubernetes API error", "details": e.body}), 500
    except Exception as e:
        return jsonify({"error": str(e)}), 500

from flask import Blueprint, request, jsonify
from kubernetes import client
from kubernetes.client.rest import ApiException
from app.services.configmap_generator import generate_configmaps_from_scenarios
from app.utils.kube_utils import load_kube_config

# Load Kubernetes configuration
load_kube_config()
core_v1 = client.CoreV1Api()

configmap_bp = Blueprint("configmap", __name__)


@configmap_bp.route("/configmaps/create", methods=["POST"])
def generate_configmaps():
    """
    Generate Kubernetes ConfigMaps based on scenario file names.

    JSON Payload:
    {
        "scenarios": ["scenario1.yaml", "scenario2.yaml"],
        "namespace": "optional-namespace"
    }
    """
    try:
        data = request.get_json()
        scenarios = data.get("scenarios")
        namespace = data.get("namespace", "default")

        if not scenarios or not isinstance(scenarios, list):
            return jsonify({"error": "Missing or invalid 'scenarios' list"}), 400

        result = generate_configmaps_from_scenarios(scenarios, namespace)
        return jsonify(result), 200

    except Exception as e:
        return jsonify({"error": "Internal server error", "details": str(e)}), 500


@configmap_bp.route("/configmaps", methods=["GET"])
def list_configmaps():
    """
    List all ConfigMaps in the specified namespace.

    Query Parameters:
    - namespace: optional, defaults to "default"
    """
    namespace = request.args.get("namespace", "default")
    results = []

    try:
        configmaps = core_v1.list_namespaced_config_map(namespace=namespace)
        for cm in configmaps.items:
            results.append(
                {
                    "name": cm.metadata.name,
                    "namespace": cm.metadata.namespace,
                    "created_at": (
                        cm.metadata.creation_timestamp.isoformat()
                        if cm.metadata.creation_timestamp
                        else None
                    ),
                }
            )

        return jsonify({"configmaps": results}), 200

    except ApiException as e:
        return jsonify({"error": "Kubernetes API error", "details": e.body}), 500
    except Exception as e:
        return jsonify({"error": "Internal server error", "details": str(e)}), 500


@configmap_bp.route("/configmaps/<string:name>", methods=["DELETE"])
def delete_configmap(name):
    """
    Delete a specific ConfigMap by name.

    URL Path:
    - name: ConfigMap name

    Query Parameters:
    - namespace: optional, defaults to "default"
    """
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
        return jsonify({"error": "Internal server error", "details": str(e)}), 500

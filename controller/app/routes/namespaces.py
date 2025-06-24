from flask import Blueprint, request, jsonify
from kubernetes import client
from kubernetes.client.rest import ApiException
from app.utils.kube_utils import load_kube_config

# Blueprint definition
namespace_bp = Blueprint("namespace", __name__)

# Initialize Kubernetes config and client
load_kube_config()
core_v1 = client.CoreV1Api()


@namespace_bp.route("/namespaces", methods=["POST"])
def create_namespace():
    """
    Create a new Kubernetes namespace.

    JSON Payload:
    {
        "name": "desired-namespace-name"
    }

    Returns:
        - 201 if created
        - 200 if already exists
        - 400 for invalid input
        - 500+ for API or internal errors
    """
    data = request.get_json()
    name = data.get("name")

    if not name:
        return jsonify({"error": "Missing 'name' field in request body"}), 400

    ns_body = client.V1Namespace(metadata=client.V1ObjectMeta(name=name))

    try:
        core_v1.create_namespace(ns_body)
        return jsonify({"namespace": name, "status": "created"}), 201

    except ApiException as e:
        if e.status == 409:
            return jsonify({"namespace": name, "status": "already_exists"}), 200
        return (
            jsonify({"error": "Failed to create namespace", "details": e.body}),
            e.status,
        )

    except Exception as e:
        return jsonify({"error": "Internal server error", "details": str(e)}), 500


@namespace_bp.route("/namespaces", methods=["GET"])
def list_namespaces():
    """
    List all namespaces in the Kubernetes cluster.

    Returns:
        - List of namespace names
        - 500+ on failure
    """
    try:
        ns_list = core_v1.list_namespace()
        namespaces = [ns.metadata.name for ns in ns_list.items]
        return jsonify({"namespaces": namespaces}), 200

    except ApiException as e:
        return (
            jsonify({"error": "Failed to list namespaces", "details": e.body}),
            e.status,
        )

    except Exception as e:
        return jsonify({"error": "Internal server error", "details": str(e)}), 500

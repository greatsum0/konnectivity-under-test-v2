from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException

namespace_bp = Blueprint("namespace", __name__)

# Kubernetes config yükle
try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()


@namespace_bp.route("/namespaces", methods=["POST"])
def create_namespace():
    data = request.get_json()
    name = data.get("name")

    if not name:
        return jsonify({"error": "Missing 'name' field"}), 400

    metadata = client.V1ObjectMeta(name=name)
    ns_spec = client.V1Namespace(metadata=metadata)

    try:
        core_v1.create_namespace(ns_spec)
        return jsonify({"namespace": name, "status": "created"}), 201

    except ApiException as e:
        if e.status == 409:  # AlreadyExists
            return jsonify({"namespace": name, "status": "already_exists"}), 200
        return (
            jsonify({"error": f"Failed to create namespace", "details": e.body}),
            e.status,
        )


@namespace_bp.route("/namespaces", methods=["GET"])
def list_namespaces():
    try:
        ns_list = core_v1.list_namespace()
        namespaces = [ns.metadata.name for ns in ns_list.items]
        return jsonify({"namespaces": namespaces}), 200

    except ApiException as e:
        return (
            jsonify({"error": "Failed to list namespaces", "details": e.body}),
            e.status,
        )

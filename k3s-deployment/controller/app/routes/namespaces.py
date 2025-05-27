from flask import Blueprint, request, jsonify
import subprocess
import json

namespaces_bp = Blueprint("namespaces", __name__)

@namespaces_bp.route("/namespaces", methods=["POST"])
def create_namespace():
    data = request.get_json()
    name = data.get("name")

    if not name:
        return jsonify({"error": "Missing 'name' field"}), 400

    try:
        subprocess.run(
            ["kubectl", "create", "namespace", name],
            check=True,
            capture_output=True,
            text=True
        )
        return jsonify({"status": "created", "namespace": name}), 201

    except subprocess.CalledProcessError as e:
        if "AlreadyExists" in e.stderr:
            return jsonify({"status": "exists", "namespace": name}), 200
        return jsonify({"error": e.stderr.strip()}), 500


@namespaces_bp.route("/namespaces", methods=["GET"])
def list_namespaces():
    try:
        result = subprocess.run(
            ["kubectl", "get", "namespaces", "-o", "json"],
            check=True,
            capture_output=True,
            text=True
        )
        output = json.loads(result.stdout)
        namespaces = [item["metadata"]["name"] for item in output.get("items", [])]
        return jsonify({"namespaces": namespaces}), 200

    except subprocess.CalledProcessError as e:
        return jsonify({"error": e.stderr.strip()}), 500

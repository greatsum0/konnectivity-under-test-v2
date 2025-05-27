from flask import Blueprint, jsonify, request
from app.services.k8s_client import list_pods

pods_bp = Blueprint("pods", __name__)


@pods_bp.route("/pods", methods=["GET"])
def get_pods():
    namespace = request.args.get("namespace")  # None means "all"
    try:
        pods = list_pods(namespace)
        return jsonify(pods), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

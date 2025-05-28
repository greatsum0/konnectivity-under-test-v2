from flask import Blueprint, request, jsonify
from app.services.viewer_pod_generator import generate_viewer_pods_from_params

viewer_pods_bp = Blueprint("viewer_pods", __name__)


@viewer_pods_bp.route("/pods/viewer/generate", methods=["POST"])
def generate_viewer_pods():
    try:
        data = request.get_json()
        namespace = data.get("namespace", "default")
        pod_list = data.get("pods")

        if not pod_list or not isinstance(pod_list, list):
            return jsonify({"error": "Missing or invalid 'pods' list"}), 400

        result = generate_viewer_pods_from_params(pod_list, namespace)
        return jsonify(result), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500

from flask import Blueprint, request, jsonify
from app.services.configmap_generator import generate_configmaps_from_uploaded_files

configmap_bp = Blueprint("configmap", __name__)

@configmap_bp.route("/configmaps/generate", methods=["POST"])
def generate_configmaps():
    try:
        if "files" not in request.files:
            return jsonify({"error": "No files uploaded"}), 400

        files = request.files.getlist("files")
        namespace = request.form.get("namespace", "default")

        result = generate_configmaps_from_uploaded_files(files, namespace)
        return jsonify(result), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500

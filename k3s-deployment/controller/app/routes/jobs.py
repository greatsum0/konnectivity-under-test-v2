from flask import Blueprint, request, jsonify
from app.services.job_generator import generate_jobs_from_uploaded_files

jobs_bp = Blueprint("jobs", __name__)


@jobs_bp.route("/jobs/generate", methods=["POST"])
def generate_jobs():
    try:
        if "files" not in request.files:
            return jsonify({"error": "No files uploaded"}), 400

        files = request.files.getlist("files")
        namespace = request.form.get("namespace", "default")

        result = generate_jobs_from_uploaded_files(files, namespace)
        return jsonify(result), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500

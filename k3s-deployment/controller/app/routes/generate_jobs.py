from flask import Blueprint, request, jsonify
from app.services.job_generator import generate_jobs_from_uploaded_files
from pathlib import Path

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


@jobs_bp.route("/jobs", methods=["GET"])
def list_jobs():
    JOBS_DIR = Path(__file__).resolve().parent.parent.parent / "jobs"

    if not JOBS_DIR.exists():
        return jsonify([])

    files = [f.name for f in JOBS_DIR.glob("*.yaml")]
    return jsonify({"files": files}), 200

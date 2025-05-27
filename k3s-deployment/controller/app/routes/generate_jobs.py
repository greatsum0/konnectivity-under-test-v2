from flask import Blueprint, request, jsonify
from app.services.job_generator import generate_jobs_from_scenario_names
from pathlib import Path

jobs_bp = Blueprint("jobs", __name__)


@jobs_bp.route("/jobs/generate", methods=["POST"])
def generate_jobs():
    try:
        data = request.get_json()
        scenarios = data.get("scenarios")
        namespace = data.get("namespace", "default")

        if not scenarios or not isinstance(scenarios, list):
            return jsonify({"error": "Missing or invalid 'scenarios' list"}), 400

        result = generate_jobs_from_scenario_names(scenarios, namespace)
        return jsonify(result), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500


@jobs_bp.route("/jobs", methods=["GET"])
def list_jobs():
    JOBS_DIR = Path(__file__).resolve().parent.parent.parent / "jobs"

    if not JOBS_DIR.exists():
        return jsonify({"files": []}), 200

    files = [f.name for f in JOBS_DIR.glob("*.yaml")]
    return jsonify({"files": files}), 200

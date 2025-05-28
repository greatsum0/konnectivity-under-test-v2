from flask import Blueprint, request, jsonify
from app.services.configmap_generator import generate_configmaps_from_scenarios
from pathlib import Path

configmap_bp = Blueprint("configmap", __name__)


@configmap_bp.route("/configmaps/generate", methods=["POST"])
def generate_configmaps():
    try:
        data = request.get_json()
        scenarios = data.get("scenarios")
        namespace = data.get("namespace", "default")

        if not scenarios or not isinstance(scenarios, list):
            return jsonify({"error": "Missing or invalid 'scenarios' list"}), 400

        result = generate_configmaps_from_scenarios(scenarios, namespace)
        return jsonify(result), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500


@configmap_bp.route("/configmaps", methods=["GET"])
def list_configmaps():
    CONFIGMAP_DIR = Path(__file__).resolve().parent.parent.parent / "configmaps"
    if not CONFIGMAP_DIR.exists():
        return jsonify({"files": []}), 200

    files = [f.name for f in CONFIGMAP_DIR.glob("*.yaml")]
    return jsonify({"files": files}), 200

from flask import Blueprint, request, jsonify
from app.services.deployment_generator import (
    generate_server_deployments_from_scenario_names,
)

deployments_bp = Blueprint("deployments", __name__)


@deployments_bp.route("/deployments/generate", methods=["POST"])
def generate_deployments():
    try:
        data = request.get_json()
        namespace = data.get("namespace")
        scenarios = data.get("scenarios")

        if not namespace:
            return jsonify({"error": "Missing 'namespace' field"}), 400
        if not scenarios or not isinstance(scenarios, list):
            return jsonify({"error": "Missing or invalid 'scenarios' list"}), 400

        result = generate_server_deployments_from_scenario_names(scenarios, namespace)
        return jsonify(result), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500

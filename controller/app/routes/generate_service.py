from flask import Blueprint, request, jsonify
from app.services.service_generator import generate_services_from_scenarios

services_bp = Blueprint("services", __name__)


@services_bp.route("/services/generate", methods=["POST"])
def generate_services():
    try:
        data = request.get_json()
        scenarios = data.get("scenarios")
        ports = data.get("ports")
        namespace = data.get("namespace", "default")

        if not scenarios or not isinstance(scenarios, list):
            return jsonify({"error": "Missing or invalid 'scenarios' list"}), 400
        if not ports or not isinstance(ports, list):
            return jsonify({"error": "Missing or invalid 'ports' list"}), 400

        result = generate_services_from_scenarios(scenarios, ports, namespace)
        return jsonify(result), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500

from flask import Blueprint, request, jsonify
from app.services.settings_manager import settings_manager

settings_bp = Blueprint("settings", __name__)

@settings_bp.route("/settings", methods=["GET"])
def get_settings():
    return jsonify(settings_manager.get()), 200

@settings_bp.route("/settings", methods=["POST"])
def update_settings():
    try:
        data = request.get_json()
        settings_manager.update(data)
        return jsonify({"status": "updated", "settings": settings_manager.get()}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

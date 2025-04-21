from flask import request, jsonify
from ..logger import CustomLogger
from pathlib import Path
from run_changed import run_changed_jobs

logger = CustomLogger("trigger", Path(__file__).parent.parent / "system.log")

def trigger_scenarios():
    data = request.get_json()
    if not data:
        return jsonify({"error": "No JSON received"}), 400

    logger.info("Webhook received.")
    run_changed_jobs(data)
    return jsonify({"status": "triggered"}), 200

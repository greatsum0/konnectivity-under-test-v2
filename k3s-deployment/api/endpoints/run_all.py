from flask import jsonify
from pathlib import Path
from ..endpoints.run_changed import run_changed_jobs
from ..logger import CustomLogger

logger = CustomLogger("run_all", Path(__file__).parent.parent / "system.log")

def run_all_jobs():
    scenario_dir = Path(__file__).parent.parent.parent / "scenarios"
    if not scenario_dir.exists():
        logger.error(f"Scenarios directory not found at {scenario_dir}")
        return jsonify({"error": "Scenario directory not found"}), 500

    all_files = [f.name for f in scenario_dir.glob("*.yaml")]
    if not all_files:
        logger.warning("No scenario files found.")
        return jsonify({"status": "no jobs to run"}), 200

    logger.info("Running all available jobs...")
    data = {"changed_files": all_files}
    run_changed_jobs(data)

    return jsonify({"status": "all jobs triggered"}), 200

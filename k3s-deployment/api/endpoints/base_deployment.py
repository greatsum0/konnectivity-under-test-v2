from flask import jsonify
from pathlib import Path
from ..logger import CustomLogger
from reset import perform_reset
from base_deployment_configmaps import generate_configmaps
from base_deployment_jobs import generate_jobs
from docker_auth import perform_docker_auth
from deploy import perform_deploy

logger = CustomLogger("base_deployment", Path(__file__).parent.parent / "system.log")

def base_deployment():
    logger.info("Starting base deployment process...")

    # Step 1: Reset cluster
    logger.info("[1/4] Resetting cluster...")
    if not perform_reset():
        logger.warning("Cluster reset failed or namespace not found. Continuing anyway...")

    # Step 2: Generate manifests
    logger.info("[2/4] Building manifests...")
    try:
        generate_configmaps()
        generate_jobs()
    except Exception as e:
        logger.error(f"Error during manifest generation: {e}")
        return jsonify({"error": "Manifest generation failed"}), 500

    # Step 3: Docker authentication
    logger.info("[3/4] Running Docker authentication...")
    if not perform_docker_auth():
        return jsonify({"error": "Docker authentication failed"}), 500


    # Step 4: Deploy base components
    logger.info("[4/4] Deploying base components...")
    if not perform_deploy():
        return jsonify({"error": "Base deployment failed"}), 500


    logger.info("Base deployment complete.")
    return jsonify({"status": "base deployment complete"}), 200

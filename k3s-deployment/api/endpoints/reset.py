# endpoints/reset.py

from flask import jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException
from env_loader import load_environment, get_env
from logger import CustomLogger
from pathlib import Path

logger = CustomLogger("reset", Path(__file__).parent.parent / "system.log")

def perform_reset():
    load_environment()

    try:
        config.load_kube_config()
    except Exception as e:
        logger.error(f"Failed to load Kubernetes config: {e}")
        return False

    namespace = get_env("NAMESPACE", "kut-app")
    logger.info(f"Resetting namespace: {namespace}")

    try:
        v1 = client.CoreV1Api()

        try:
            v1.delete_namespace(name=namespace)
            logger.info(f"Deletion requested for namespace '{namespace}'")
        except ApiException as e:
            if e.status == 404:
                logger.warning(f"Namespace '{namespace}' not found, skipping deletion.")
            else:
                logger.error(f"Error deleting namespace '{namespace}': {e}")
                return False

        try:
            v1.read_namespace(name=namespace)
            while True:
                try:
                    v1.read_namespace(name=namespace)
                except ApiException as e:
                    if e.status == 404:
                        logger.info(f"Namespace '{namespace}' deleted successfully.")
                        break
                    else:
                        logger.error(f"Unexpected error while checking namespace status: {e}")
                        return False
        except ApiException as e:
            if e.status == 404:
                logger.info(f"Namespace '{namespace}' was already deleted.")
            else:
                logger.error(f"Error while checking namespace: {e}")
                return False

        # Clean up Released PVs
        pvs = v1.list_persistent_volume().items
        for pv in pvs:
            if pv.status.phase == "Released":
                logger.info(f"Deleting released PV: {pv.metadata.name}")
                v1.delete_persistent_volume(name=pv.metadata.name)

        logger.info("Reset complete.")
        return True

    except ApiException as e:
        logger.error(f"Kubernetes API error: {e}")
        return False

# HTTP endpoint version
def reset_cluster():
    success = perform_reset()
    if success:
        return jsonify({"status": "reset", "exit_code": 0}), 200
    else:
        return jsonify({"error": "Reset failed"}), 500

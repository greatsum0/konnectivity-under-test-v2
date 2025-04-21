from env_loader import get_env
from logger import CustomLogger
from pathlib import Path
import subprocess

logger = CustomLogger("base_deployment", Path(__file__).parent.parent / "system.log")

def perform_docker_auth():
    username = get_env("AUTH_USERNAME")
    token = get_env("AUTH_TOKEN")
    namespace = get_env("NAMESPACE")

    if not username or not token or not namespace:
        logger.error("AUTH_USERNAME or AUTH_TOKEN or NAMESPACE not set in .env file!")
        return False

    try:
        # Create namespace if it doesn't exist
        logger.info(f"Ensuring namespace '{namespace}' exists...")
        create_ns = subprocess.run(
            ["kubectl", "create", "namespace", namespace, "--dry-run=client", "-o", "yaml"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        subprocess.run(["kubectl", "apply", "-f", "-"], input=create_ns.stdout, text=True)

        # Create or update docker-regcred
        logger.info("Creating/updating Docker auth secret...")
        create_secret = subprocess.run(
            [
                "kubectl", "create", "secret", "docker-registry", "kut-regcred",
                f"--docker-username={username}",
                f"--docker-password={token}",
                "-n", namespace,
                "--dry-run=client", "-o", "yaml"
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        subprocess.run(["kubectl", "apply", "-f", "-"], input=create_secret.stdout, text=True)

        logger.info("Auth secret created/updated successfully.")
        return True

    except Exception as e:
        logger.error(f"Exception during Docker auth: {e}")
        return False

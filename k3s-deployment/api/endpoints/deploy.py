from logger import CustomLogger
from env_loader import get_env
from pathlib import Path
import subprocess

logger = CustomLogger("base_deployment", Path(__file__).parent.parent / "system.log")

def perform_deploy():
    base_dir = Path(__file__).parent.parent.parent
    namespace = get_env("NAMESPACE", "kut-app")

    try:
        logger.info("[1/3] Creating namespace...")
        result = subprocess.run(
            ["kubectl", "apply", "-f", str(base_dir / "namespace.yaml")],
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            logger.error(result.stdout + result.stderr)
            return False
        logger.info(result.stdout.strip())

        logger.info("[2/3] Applying base manifests (config, PVC, deployments, services)...")

        kustomize_dirs = ["config", "configmaps", "deployments", "pvc", "services", "tools"]
        for d in kustomize_dirs:
            path = base_dir / d
            logger.info(f"Applying: {path}")
            result = subprocess.run(
                ["kubectl", "apply", "-k", str(path)],
                capture_output=True,
                text=True
            )
            if result.returncode != 0:
                logger.error(result.stdout + result.stderr)
                return False
            logger.info(result.stdout.strip())

        logger.info("[3/3] Waiting for kut-server to become ready...")

        result = subprocess.run(
            [
                "kubectl", "wait",
                "--for=condition=available",
                "deployment/kut-server",
                "-n", namespace,
                "--timeout=60s"
            ],
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            logger.error("Timeout waiting for kut-server to become available.")
            logger.error(result.stdout + result.stderr)
            return False

        logger.info(result.stdout.strip())
        logger.info("Base deployment completed.")
        return True

    except Exception as e:
        logger.error(f"Exception during deployment: {e}")
        return False

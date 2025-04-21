import subprocess
import time
from pathlib import Path
from ..logger import CustomLogger
from ..env_loader import get_env

logger = CustomLogger("run_changed", Path(__file__).parent.parent / "system.log")

def run_changed_jobs(data: dict):
    namespace = get_env("NAMESPACE", "kut-app")
    base_dir = Path(__file__).parent.parent.parent
    job_dir = base_dir / "jobs"

    changed_files = data.get("changed_files", [])
    if not changed_files:
        logger.warning("No changed files found in webhook payload.")
        return

    logger.info("Changed files detected:")
    for f in changed_files:
        logger.info(f" - {f}")

    for file in changed_files:
        job_name = Path(file).stem
        job_manifest = job_dir / f"job-{job_name}.yaml"

        if not job_manifest.exists():
            logger.warning(f"Job manifest not found for '{job_name}' — skipping.")
            continue

        logger.info(f"Applying job: {job_name}")

        # Delete previous job (if exists)
        subprocess.run(
            ["kubectl", "delete", "job", f"kut-client-{job_name}", "-n", namespace,
             "--ignore-not-found", "--wait=true"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )

        # Apply job manifest
        subprocess.run(
            ["kubectl", "apply", "-f", str(job_manifest), "-n", namespace],
            check=True
        )

        logger.info(f"Waiting for pod of job 'kut-client-{job_name}' to start...")
        pod_name = None
        while not pod_name:
            try:
                result = subprocess.run(
                    [
                        "kubectl", "get", "pods", "-n", namespace,
                        "-l", f"job-name=kut-client-{job_name}",
                        "-o", "jsonpath={.items[0].metadata.name}"
                    ],
                    capture_output=True, text=True
                )
                pod_name = result.stdout.strip()
            except Exception:
                pass
            time.sleep(1)

        logger.info(f"Pod '{pod_name}' created. Waiting for it to complete...")

        while True:
            phase_result = subprocess.run(
                ["kubectl", "get", "pod", pod_name, "-n", namespace, "-o", "jsonpath={.status.phase}"],
                capture_output=True,
                text=True
            )
            phase = phase_result.stdout.strip()
            if phase == "Succeeded":
                logger.info(f"Pod '{pod_name}' completed successfully.")
                break
            elif phase == "Failed":
                logger.error(f"Pod '{pod_name}' failed.")
                break
            time.sleep(2)

        # Fetch logs
        logger.info(f"Fetching logs for pod '{pod_name}'...")
        log_result = subprocess.run(
            ["kubectl", "logs", pod_name, "-n", namespace],
            capture_output=True, text=True
        )
        logger.info(log_result.stdout.strip())

        logger.info(f"Job 'kut-client-{job_name}' finished.")

    logger.info("Selected scenario jobs completed successfully.")

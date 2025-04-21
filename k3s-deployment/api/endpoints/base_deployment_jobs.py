from env_loader import get_env
from logger import CustomLogger
from pathlib import Path
import yaml

logger = CustomLogger("base_deployment_jobs", Path(__file__).parent.parent / "system.log")

def generate_jobs():
    base_dir = Path(__file__).parent.parent.parent  # k3s-deployment/
    print("base_dir", base_dir)
    scenario_dir = base_dir / "scenarios"
    jobs_dir = base_dir / "jobs"
    template_path = base_dir / "templates" / "job-template.yaml"
    namespace = get_env("NAMESPACE", "kut-app")
    image_name = get_env("IMAGE_NAME")

    if not image_name:
        logger.error("IMAGE_NAME environment variable is not set.")
        return

    logger.info("Preparing jobs directory...")
    jobs_dir.mkdir(parents=True, exist_ok=True)

    logger.info(f"Generating Job manifests from '{scenario_dir}'...")

    job_files = []

    template = template_path.read_text()

    for scenario_file in scenario_dir.glob("*.yaml"):
        name = scenario_file.stem
        job_file = jobs_dir / f"job-{name}.yaml"

        logger.info(f"Creating job manifest: {job_file.name}")

        content = (
            template
            .replace("<scenario-name>", name)
            .replace("<scenario-file>", name)
            .replace("placeholder/temp-image", image_name)
        )

        with open(job_file, "w") as f:
            f.write(content)

        job_files.append(job_file.name)

    # Generate kustomization.yaml
    kustomization_path = jobs_dir / "kustomization.yaml"
    logger.info(f"Writing kustomization.yaml to {kustomization_path}")

    kustomization = {
        "apiVersion": "kustomize.config.k8s.io/v1beta1",
        "kind": "Kustomization",
        "namespace": namespace,
        "resources": job_files
    }

    with open(kustomization_path, "w") as f:
        yaml.dump(kustomization, f)

    logger.info(f"All job manifests created under '{jobs_dir}'")

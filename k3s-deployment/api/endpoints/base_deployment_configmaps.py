from ..env_loader import get_env
from ..logger import CustomLogger
from pathlib import Path
import subprocess
import yaml

logger = CustomLogger("base_deployment_configmaps", Path(__file__).parent.parent / "system.log")

def generate_configmaps():
    base_dir = Path(__file__).parent.parent.parent  # k3s-deployment/
    scenario_dir = base_dir / "scenarios"
    configmap_dir = base_dir / "configmaps"
    namespace = get_env("NAMESPACE", "kut-app")

    logger.info("Preparing configmaps directory...")
    configmap_dir.mkdir(parents=True, exist_ok=True)

    logger.info(f"Generating ConfigMaps from scenarios in '{scenario_dir}'...")

    configmap_paths = []

    for scenario_file in scenario_dir.glob("*.yaml"):
        name = scenario_file.stem
        configmap_name = f"kut-client-{name}-config"
        output_path = configmap_dir / f"{configmap_name}.yaml"

        logger.info(f"Generating ConfigMap: {configmap_name} → {output_path}")

        cmd = [
            "kubectl", "create", "configmap", configmap_name,
            "--from-file", f"scenario.yaml={str(scenario_file)}",
            "--namespace", namespace,
            "--dry-run=client", "-o", "yaml"
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            logger.error(f"Failed to generate configmap for {name}: {result.stderr}")
            continue

        with open(output_path, "w") as f:
            f.write(result.stdout)

        configmap_paths.append(output_path.name)

    # Generate kustomization.yaml
    kustomization_path = configmap_dir / "kustomization.yaml"
    logger.info(f"Writing kustomization.yaml to {kustomization_path}")

    kustomization = {
        "apiVersion": "kustomize.config.k8s.io/v1beta1",
        "kind": "Kustomization",
        "namespace": namespace,
        "resources": configmap_paths
    }

    with open(kustomization_path, "w") as f:
        yaml.dump(kustomization, f)

    logger.info(f"All ConfigMaps generated in '{configmap_dir}'")

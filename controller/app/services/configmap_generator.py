from typing import List
import yaml
from kubernetes import client
from kubernetes.client.rest import ApiException

from app.utils.dir_utils import get_dir
from app.utils.kube_utils import load_kube_config

# Load Kubernetes config using the utility (local or in-cluster)
load_kube_config()
core_v1 = client.CoreV1Api()


def generate_configmaps_from_scenarios(
    scenarios: List[str], namespace: str = "default"
):
    """
    Generate Kubernetes ConfigMaps from given scenario YAML files.

    Args:
        scenarios (List[str]): List of scenario YAML filenames
        namespace (str): Kubernetes namespace where ConfigMaps will be applied

    Returns:
        dict: Summary of creation/replacement results
    """
    scenario_dir = get_dir("scenarios")
    output_dir = get_dir("configmaps")
    results = []

    for scenario_file in scenarios:
        scenario_path = scenario_dir / scenario_file
        stem = scenario_path.stem
        configmap_name = f"kut-{stem}-config"

        if not scenario_path.exists():
            results.append(
                {
                    "name": configmap_name,
                    "status": "failed",
                    "error": f"File '{scenario_file}' not found",
                }
            )
            continue

        try:
            file_content = scenario_path.read_text()
        except Exception as e:
            results.append(
                {
                    "name": configmap_name,
                    "status": "failed",
                    "error": f"Failed to read '{scenario_file}': {str(e)}",
                }
            )
            continue

        metadata = client.V1ObjectMeta(name=configmap_name, namespace=namespace)
        config_map = client.V1ConfigMap(
            metadata=metadata, data={"scenario.yaml": file_content}
        )

        # Create or replace ConfigMap in cluster
        try:
            core_v1.read_namespaced_config_map(configmap_name, namespace)
            core_v1.replace_namespaced_config_map(configmap_name, namespace, config_map)
            action = "replaced"
        except ApiException as e:
            if e.status == 404:
                core_v1.create_namespaced_config_map(namespace, config_map)
                action = "created"
            else:
                results.append(
                    {
                        "name": configmap_name,
                        "status": "failed",
                        "error": f"Kubernetes API error: {e.body}",
                    }
                )
                continue

        # Save ConfigMap as YAML locally
        output_path = output_dir / f"{configmap_name}.yaml"
        try:
            yaml_output = {
                "apiVersion": "v1",
                "kind": "ConfigMap",
                "metadata": {"name": configmap_name, "namespace": namespace},
                "data": {"scenario.yaml": file_content},
            }
            with open(output_path, "w") as f:
                yaml.dump(yaml_output, f)
        except Exception as e:
            results.append(
                {
                    "name": configmap_name,
                    "status": action,
                    "warning": f"ConfigMap applied, but failed to write file: {str(e)}",
                }
            )
            continue

        results.append(
            {"name": configmap_name, "output_path": str(output_path), "status": action}
        )

    return {"namespace": namespace, "results": results}

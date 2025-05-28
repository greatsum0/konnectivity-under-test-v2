from kubernetes import client, config
from kubernetes.client.rest import ApiException
from pathlib import Path
import tempfile
import yaml
from werkzeug.datastructures import FileStorage

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()


def generate_configmaps_from_uploaded_files(files: list[FileStorage], namespace: str):
    OUTPUT_DIR = Path(__file__).resolve().parent.parent.parent / "configmaps"
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    results = []
    for file in files:
        filename = Path(file.filename)
        stem = filename.stem
        configmap_name = f"kut-{stem}-config"

        file_content = file.read().decode("utf-8")

        metadata = client.V1ObjectMeta(name=configmap_name, namespace=namespace)
        config_map = client.V1ConfigMap(
            metadata=metadata, data={"scenario.yaml": file_content}
        )

        try:
            existing = core_v1.read_namespaced_config_map(configmap_name, namespace)
            core_v1.replace_namespaced_config_map(configmap_name, namespace, config_map)
            action = "replaced"
        except ApiException as e:
            if e.status == 404:
                core_v1.create_namespaced_config_map(namespace, config_map)
                action = "created"
            else:
                results.append(
                    {"name": configmap_name, "status": "failed", "error": e.body}
                )
                continue

        output_path = OUTPUT_DIR / f"{configmap_name}.yaml"
        yaml_output = {
            "apiVersion": "v1",
            "kind": "ConfigMap",
            "metadata": {"name": configmap_name, "namespace": namespace},
            "data": {"scenario.yaml": file_content},
        }
        with open(output_path, "w") as f:
            yaml.dump(yaml_output, f)

        results.append(
            {
                "name": configmap_name,
                "output_path": str(output_path),
                "status": action,
            }
        )

    return {"results": results}

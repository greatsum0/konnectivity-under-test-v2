from kubernetes import client, config
from kubernetes.client.rest import ApiException
from pathlib import Path
import tempfile
import yaml
from werkzeug.datastructures import FileStorage

# Kubernetes config yükle
try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()

OUTPUT_DIR = Path(__file__).resolve().parent.parent.parent / "configmaps"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def generate_configmaps_from_uploaded_files(files: list[FileStorage], namespace: str):
    results = []

    for file in files:
        filename = Path(file.filename)
        stem = filename.stem
        configmap_name = f"kut-client-{stem}-config"

        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            file_content = file.read().decode("utf-8")
            tmp_file.write(file_content)
            tmp_file_path = tmp_file.name

        metadata = client.V1ObjectMeta(name=configmap_name, namespace=namespace)
        config_map = client.V1ConfigMap(
            metadata=metadata, data={"scenario.yaml": file_content}
        )

        try:
            core_v1.create_namespaced_config_map(namespace=namespace, body=config_map)

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
                    "status": "created",
                }
            )

        except ApiException as e:
            results.append(
                {"name": configmap_name, "status": "failed", "error": e.body}
            )

    return {"created": results}

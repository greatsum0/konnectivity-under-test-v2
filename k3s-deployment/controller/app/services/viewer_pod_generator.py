from kubernetes import client, config
from kubernetes.client.rest import ApiException
from pathlib import Path
import yaml

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()


def generate_viewer_pods_from_params(
    pod_configs: list[dict], namespace: str = "default"
):
    """
        - pod_name
        - pvc_name
        - node_name
    """
    TEMPLATE_PATH = (
        Path(__file__).resolve().parent.parent.parent
        / "templates"
        / "viewer-pod-template.yaml"
    )
    OUTPUT_DIR = Path(__file__).resolve().parent.parent.parent / "viewer-pods"
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    template_raw = TEMPLATE_PATH.read_text()
    results = []

    for config_item in pod_configs:
        pod_name = config_item.get("pod_name")
        pvc_name = config_item.get("pvc_name")
        node_name = config_item.get("node_name")

        if not pod_name or not pvc_name or not node_name:
            results.append(
                {
                    "status": "skipped",
                    "reason": "Missing pod_name, pvc_name, or node_name",
                    "pod": config_item,
                }
            )
            continue

        try:
            pod_yaml_str = (
                template_raw.replace("<pod-name>", pod_name)
                .replace("<namespace>", namespace)
                .replace("<node-name>", node_name)
                .replace("<pvc-name>", pvc_name)
            )

            pod_manifest = yaml.safe_load(pod_yaml_str)

            try:
                core_v1.read_namespaced_pod(pod_name, namespace)
                core_v1.replace_namespaced_pod(pod_name, namespace, pod_manifest)
                action = "replaced"
            except ApiException as e:
                if e.status == 404:
                    core_v1.create_namespaced_pod(namespace, pod_manifest)
                    action = "created"
                else:
                    raise e

            output_path = OUTPUT_DIR / f"{pod_name}.yaml"
            with open(output_path, "w") as f:
                yaml.dump(pod_manifest, f)

            results.append(
                {
                    "pod": pod_name,
                    "pvc": pvc_name,
                    "node": node_name,
                    "status": action,
                    "output_path": str(output_path),
                }
            )

        except Exception as e:
            results.append({"pod": pod_name, "status": "failed", "error": str(e)})

    return {"namespace": namespace, "results": results}

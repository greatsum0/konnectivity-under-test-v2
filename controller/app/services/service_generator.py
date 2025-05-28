from pathlib import Path
from kubernetes import client, config
from kubernetes.client.rest import ApiException
import yaml

# Kubernetes config yükle
try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()


def generate_services_from_scenarios(
    scenarios: list[str], ports: list[dict], namespace: str = "default"
):

    OUTPUT_DIR = Path(__file__).resolve().parent.parent.parent / "services"
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    results = []

    for scenario in scenarios:
        service_name = f"kut-service-{scenario}"
        filename = f"service-{scenario}.yaml"
        output_path = OUTPUT_DIR / filename

        service_manifest = {
            "apiVersion": "v1",
            "kind": "Service",
            "metadata": {
                "name": service_name,
                "namespace": namespace,
            },
            "spec": {
                "selector": {"app": f"kut-deploy-{scenario}"},
                "ports": [
                    {
                        "name": port["name"],
                        "protocol": port.get("protocol", "TCP"),
                        "port": port["port"],
                        "targetPort": port.get("targetPort", port["port"]),
                    }
                    for port in ports
                ],
                "type": "ClusterIP",
            },
        }

        try:
            try:
                core_v1.read_namespaced_service(service_name, namespace)
                core_v1.replace_namespaced_service(
                    service_name, namespace, service_manifest
                )
                action = "replaced"
            except ApiException as e:
                if e.status == 404:
                    core_v1.create_namespaced_service(namespace, service_manifest)
                    action = "created"
                else:
                    raise

            with open(output_path, "w") as f:
                yaml.dump(service_manifest, f)

            results.append(
                {
                    "name": service_name,
                    "output_path": str(output_path),
                    "status": action,
                }
            )

        except ApiException as e:
            results.append({"name": service_name, "status": "failed", "error": e.body})
        except Exception as e:
            results.append({"name": service_name, "status": "failed", "error": str(e)})

    return {"namespace": namespace, "created": results}

from pathlib import Path
from kubernetes import client, config
from kubernetes.client.rest import ApiException
import yaml

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()

TEMPLATE_PATH = (
    Path(__file__).resolve().parent.parent.parent
    / "templates"
    / "service-template.yaml"
)
OUTPUT_DIR = Path(__file__).resolve().parent.parent.parent / "services"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def generate_services_from_scenarios(
    scenarios: list[str], ports: list[dict], namespace: str = "default"
):
    template_raw = TEMPLATE_PATH.read_text()
    results = []

    port_lines = []
    for port in ports:
        port_lines.append(
            f"    - name: {port['name']}\n"
            f"      protocol: {port.get('protocol', 'TCP')}\n"
            f"      port: {port['port']}\n"
            f"      targetPort: {port.get('targetPort', port['port'])}"
        )
    ports_block = "\n".join(port_lines)

    for scenario in scenarios:
        service_name = f"kut-server-{scenario}"
        filename = f"service-{scenario}.yaml"
        output_path = OUTPUT_DIR / filename

        try:
            service_yaml = (
                template_raw.replace("<scenario-name>", scenario)
                .replace("<namespace>", namespace)
                .replace("<port-definitions>", ports_block)
            )

            service_manifest = yaml.safe_load(service_yaml)
            service_manifest["metadata"]["namespace"] = namespace

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

from pathlib import Path
from app.services.settings_manager import settings_manager
from kubernetes import client, config
from kubernetes.client.rest import ApiException
import yaml

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

apps_v1 = client.AppsV1Api()


def generate_server_deployments_from_scenario_names(
    scenarios: list[str], namespace: str = "default"
):
    OUTPUT_DIR = Path(__file__).resolve().parent.parent.parent / "deployments"
    TEMPLATE_PATH = (
        Path(__file__).resolve().parent.parent.parent
        / "templates"
        / "server-template.yaml"
    )

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    settings = settings_manager.get()
    image_name = settings.get("IMAGE_NAME")
    image_tag = settings.get("IMAGE_TAG", "latest")

    if not image_name:
        return {"error": "IMAGE_NAME is not set in settings."}

    full_image = f"{image_name}:{image_tag}"
    template_raw = TEMPLATE_PATH.read_text()
    results = []

    for scenario in scenarios:
        deployment_name = f"kut-deploy-{scenario}"
        filename = f"deployment-{scenario}.yaml"
        output_path = OUTPUT_DIR / filename

        try:
            deployment_yaml = template_raw.replace("<scenario-name>", scenario).replace(
                "placeholder/temp-image", full_image
            )

            deployment_manifest = yaml.safe_load(deployment_yaml)
            deployment_manifest["metadata"]["namespace"] = namespace

            try:
                apps_v1.read_namespaced_deployment(deployment_name, namespace)
                apps_v1.replace_namespaced_deployment(
                    deployment_name, namespace, deployment_manifest
                )
                action = "replaced"
            except ApiException as e:
                if e.status == 404:
                    apps_v1.create_namespaced_deployment(
                        namespace=namespace, body=deployment_manifest
                    )
                    action = "created"
                else:
                    raise e

            with open(output_path, "w") as f:
                yaml.dump(deployment_manifest, f)

            results.append(
                {
                    "name": deployment_name,
                    "output_path": str(output_path),
                    "status": action,
                }
            )

        except ApiException as e:
            results.append(
                {"name": deployment_name, "status": "failed", "error": e.body}
            )
        except Exception as e:
            results.append(
                {"name": deployment_name, "status": "failed", "error": str(e)}
            )

    return {"namespace": namespace, "created": results}

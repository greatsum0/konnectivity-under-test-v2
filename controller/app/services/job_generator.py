from pathlib import Path
from app.services.settings_manager import settings_manager
from kubernetes import client, config
from kubernetes.client.rest import ApiException
import yaml

# Kubernetes config
try:
    config.load_kube_config()
except:
    config.load_incluster_config()

batch_v1 = client.BatchV1Api()

# Paths
BASE_DIR = Path(__file__).resolve().parent.parent.parent
OUTPUT_DIR = BASE_DIR / "jobs"
TEMPLATE_PATH = BASE_DIR / "templates" / "job-template.yaml"

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def generate_jobs_from_scenario_names(scenarios: list[str], namespace: str = "kut-app"):
    settings = settings_manager.get()
    image_name = settings.get("IMAGE_NAME")
    image_tag = settings.get("IMAGE_TAG", "latest")

    if not image_name:
        return {"error": "IMAGE_NAME is not set in settings."}

    full_image = f"{image_name}:{image_tag}"
    template_raw = TEMPLATE_PATH.read_text()
    results = []

    for scenario in scenarios:
        job_name = f"kut-client-{scenario}"
        job_filename = f"job-{scenario}.yaml"
        output_path = OUTPUT_DIR / job_filename
        print(f"Generating job for scenario: {scenario}")
        try:
            job_yaml = (
                template_raw.replace("<scenario-name>", scenario)
                .replace("<scenario-file>", scenario)
                .replace("placeholder/temp-image", full_image)
            )

            job_manifest = yaml.safe_load(job_yaml)
            job_manifest["metadata"]["namespace"] = namespace

            batch_v1.create_namespaced_job(namespace=namespace, body=job_manifest)

            with open(output_path, "w") as f:
                yaml.dump(job_manifest, f)

            results.append(
                {
                    "scenario": scenario,
                    "job_name": job_name,
                    "output_path": str(output_path),
                    "status": "created",
                }
            )

        except ApiException as e:
            results.append(
                {
                    "scenario": scenario,
                    "job_name": job_name,
                    "status": "failed",
                    "error": e.body,
                }
            )
        except Exception as e:
            results.append(
                {
                    "scenario": scenario,
                    "job_name": job_name,
                    "status": "failed",
                    "error": str(e),
                }
            )

    return {"namespace": namespace, "created": results}

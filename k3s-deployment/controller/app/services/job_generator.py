from pathlib import Path
from werkzeug.datastructures import FileStorage
from app.services.settings_manager import (
    settings_manager,
)  # yeni: ayarlar buradan alınır

OUTPUT_DIR = Path(__file__).resolve().parent.parent.parent / "jobs"
TEMPLATE_PATH = (
    Path(__file__).resolve().parent.parent.parent / "templates" / "job-template.yaml"
)

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def generate_jobs_from_uploaded_files(files: list[FileStorage], namespace: str = None):
    # settings.yaml üzerinden ayarları al
    settings = settings_manager.get()

    image_name = settings.get("IMAGE_NAME")
    image_tag = settings.get("IMAGE_TAG", "latest")

    if not image_name:
        return {"error": "IMAGE_NAME is not set in settings."}

    full_image = f"{image_name}:{image_tag}"
    results = []

    template = TEMPLATE_PATH.read_text()

    for file in files:
        filename = Path(file.filename)
        stem = filename.stem
        job_name = f"kut-client-{stem}"
        job_filename = f"job-{stem}.yaml"
        output_path = OUTPUT_DIR / job_filename

        try:
            # job-template.yaml'daki yer tutucuları doldur
            job_content = (
                template.replace("<scenario-name>", stem)
                .replace("<scenario-file>", stem)
                .replace("placeholder/temp-image", full_image)
            )

            # job manifestini diske yaz
            output_path.write_text(job_content)

            results.append(
                {"name": job_name, "output_path": str(output_path), "status": "success"}
            )

        except Exception as e:
            results.append({"name": job_name, "error": str(e), "status": "failed"})

    return {"namespace": namespace, "created": results}

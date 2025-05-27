import subprocess
import tempfile
from pathlib import Path
from werkzeug.datastructures import FileStorage

OUTPUT_DIR = Path(__file__).resolve().parent.parent.parent / "configmaps"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

def generate_configmaps_from_uploaded_files(files, namespace: str):
    results = []

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)

        for file in files:
            filename = Path(file.filename)
            stem = filename.stem
            configmap_name = f"kut-client-{stem}-config"
            temp_file_path = tmpdir / "scenario.yaml"

            # Dosyayı geçici yere kaydet
            file.save(temp_file_path)

            cmd = [
                "kubectl", "create", "configmap", configmap_name,
                "--from-file", f"scenario.yaml={temp_file_path}",
                "--namespace", namespace,
                "--dry-run=client", "-o", "yaml"
            ]

            try:
                output = subprocess.check_output(cmd, text=True)

                # Kalıcı olarak configmaps dizinine yaz
                output_path = OUTPUT_DIR / f"{configmap_name}.yaml"
                output_path.write_text(output)

                results.append({
                    "name": configmap_name,
                    "output_path": str(output_path),
                    "status": "success"
                })
            except subprocess.CalledProcessError as e:
                results.append({
                    "name": configmap_name,
                    "error": e.stderr,
                    "status": "failed"
                })

    return {"created": results}

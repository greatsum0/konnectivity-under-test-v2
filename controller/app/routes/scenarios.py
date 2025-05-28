from flask import Blueprint, request, jsonify
from pathlib import Path
from werkzeug.datastructures import FileStorage
from datetime import datetime

scenarios_bp = Blueprint("scenarios", __name__)

SCENARIO_DIR = Path(__file__).resolve().parent.parent.parent / "scenarios"

@scenarios_bp.route("/scenarios/upload", methods=["POST"])
def upload_scenarios():
    SCENARIO_DIR.mkdir(parents=True, exist_ok=True)
    if "files" not in request.files:
        return jsonify({"error": "No files uploaded"}), 400

    files = request.files.getlist("files")
    results = []

    for file in files:
        if not isinstance(file, FileStorage):
            continue

        file_path = SCENARIO_DIR / file.filename

        try:
            file_content = file.read().decode("utf-8")
            action = "replaced" if file_path.exists() else "created"
            file_path.write_text(file_content)

            results.append(
                {"name": file.filename, "output_path": str(file_path), "status": action}
            )

        except Exception as e:
            results.append({"name": file.filename, "status": "failed", "error": str(e)})

    return jsonify({"saved": results}), 200


@scenarios_bp.route("/scenarios", methods=["GET"])
def list_scenarios():
    if not SCENARIO_DIR.exists():
        return jsonify({"files": []}), 200

    files = []
    for f in SCENARIO_DIR.glob("*.yaml"):
        stat = f.stat()
        created_at = datetime.fromtimestamp(stat.st_ctime).isoformat()
        files.append({"name": f.name, "created_at": created_at})

    return jsonify({"files": files}), 200

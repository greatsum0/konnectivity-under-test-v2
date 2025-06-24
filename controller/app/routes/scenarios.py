from flask import Blueprint, request, jsonify
from werkzeug.datastructures import FileStorage
from werkzeug.utils import secure_filename
from app.utils.dir_utils import get_dir, dir_exists

scenarios_bp = Blueprint("scenarios", __name__)


@scenarios_bp.route("/scenarios/upload", methods=["POST"])
def upload_scenarios():
    """
    Upload one or more scenario files (.yaml) into the 'scenarios' directory.
    Existing files with the same name will be replaced.

    Accepts:
        - multipart/form-data with key 'files' and one or more file objects

    Returns:
        - JSON list of uploaded files with status (created/replaced/failed)
    """
    scenario_dir = get_dir("scenarios")  # Ensures the directory exists

    if "files" not in request.files:
        return jsonify({"error": "No files uploaded"}), 400

    files = request.files.getlist("files")
    results = []

    for file in files:
        if not isinstance(file, FileStorage):
            results.append({"status": "skipped", "reason": "invalid file object"})
            continue

        filename = secure_filename(file.filename)

        if not filename.endswith(".yaml"):
            results.append(
                {
                    "name": filename,
                    "status": "skipped",
                    "reason": "invalid file extension",
                }
            )
            continue

        file_path = scenario_dir / filename

        try:
            content = file.read().decode("utf-8")
            action = "replaced" if file_path.exists() else "created"
            file_path.write_text(content)

            results.append(
                {"name": filename, "output_path": str(file_path), "status": action}
            )

        except Exception as e:
            results.append({"name": filename, "status": "failed", "error": str(e)})

    return jsonify({"saved": results}), 200


@scenarios_bp.route("/scenarios", methods=["GET"])
def list_scenarios():
    """
    List all uploaded scenario files under the 'scenarios' directory.

    Returns:
        - List of file names (only .yaml files)
    """
    if not dir_exists("scenarios"):
        return jsonify({"files": []}), 200

    scenario_dir = get_dir("scenarios", ensure=False)
    files = [f.name for f in scenario_dir.glob("*.yaml")]

    return jsonify({"files": files}), 200

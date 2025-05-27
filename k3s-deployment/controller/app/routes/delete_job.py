from flask import Blueprint, request, jsonify
import subprocess

delete_bp = Blueprint("delete", __name__)

@delete_bp.route("/jobs/<job_name>", methods=["DELETE"])
def delete_job(job_name):
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        subprocess.run(
            ["kubectl", "delete", "job", job_name, "-n", namespace],
            check=True,
            capture_output=True,
            text=True
        )
        return jsonify({
            "status": "deleted",
            "job": job_name,
            "namespace": namespace
        }), 200

    except subprocess.CalledProcessError as e:
        return jsonify({
            "error": f"Failed to delete job '{job_name}' in namespace '{namespace}'",
            "details": e.stderr.strip()
        }), 500

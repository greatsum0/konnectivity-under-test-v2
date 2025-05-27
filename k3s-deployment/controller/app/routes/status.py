from flask import Blueprint, request, jsonify
import subprocess
import json

status_bp = Blueprint("status", __name__)

@status_bp.route("/status/<job_name>", methods=["GET"])
def get_job_status(job_name):
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        result = subprocess.run(
            ["kubectl", "get", "job", job_name, "-n", namespace, "-o", "json"],
            check=True,
            capture_output=True,
            text=True
        )
        job_json = json.loads(result.stdout)
        status = parse_job_status(job_json)
        return jsonify({
            "job": job_name,
            "namespace": namespace,
            "status": status
        })

    except subprocess.CalledProcessError as e:
        return jsonify({
            "error": f"Job '{job_name}' not found in namespace '{namespace}'",
            "details": e.stderr.strip()
        }), 404


@status_bp.route("/status", methods=["GET"])
def get_all_job_statuses():
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        result = subprocess.run(
            ["kubectl", "get", "jobs", "-n", namespace, "-o", "json"],
            check=True,
            capture_output=True,
            text=True
        )

        jobs_data = json.loads(result.stdout)
        job_items = jobs_data.get("items", [])
        statuses = []

        for job in job_items:
            name = job["metadata"]["name"]
            status = parse_job_status(job)
            statuses.append({
                "job": name,
                "namespace": namespace,
                "status": status
            })

        return jsonify({"jobs": statuses}), 200

    except subprocess.CalledProcessError as e:
        return jsonify({
            "error": "Failed to get jobs",
            "details": e.stderr.strip()
        }), 500


def parse_job_status(job_json):
    status_obj = job_json.get("status", {})
    if "succeeded" in status_obj:
        return "Completed"
    elif "failed" in status_obj:
        return "Failed"
    elif "active" in status_obj:
        return "Running"
    else:
        return "Unknown"

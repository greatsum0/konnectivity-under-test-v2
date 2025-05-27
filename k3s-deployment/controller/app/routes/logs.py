from flask import Blueprint, request, jsonify
import subprocess
import json

logs_bp = Blueprint("logs", __name__)

@logs_bp.route("/logs/<job_name>", methods=["GET"])
def get_job_logs(job_name):
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        # Step 1: Get pod name by job-name label
        result = subprocess.run(
            ["kubectl", "get", "pods", "-l", f"job-name={job_name}", "-n", namespace, "-o", "json"],
            check=True,
            capture_output=True,
            text=True
        )
        pods = json.loads(result.stdout).get("items", [])
        if not pods:
            return jsonify({"error": f"No pods found for job '{job_name}'"}), 404

        pod_name = pods[0]["metadata"]["name"]

        # Step 2: Get logs of that pod
        log_result = subprocess.run(
            ["kubectl", "logs", pod_name, "-n", namespace],
            check=True,
            capture_output=True,
            text=True
        )

        return jsonify({
            "job": job_name,
            "namespace": namespace,
            "pod": pod_name,
            "logs": log_result.stdout
        }), 200

    except subprocess.CalledProcessError as e:
        return jsonify({"error": e.stderr.strip()}), 500

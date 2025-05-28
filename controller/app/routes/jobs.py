from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException
from app.services.job_generator import generate_jobs_from_scenario_names

jobs_bp = Blueprint("jobs", __name__)

# Kube config
try:
    config.load_kube_config()
except:
    config.load_incluster_config()

batch_v1 = client.BatchV1Api()


@jobs_bp.route("/jobs/delete", methods=["DELETE"])
def delete_jobs():
    data = request.get_json()
    namespace = data.get("namespace")
    jobs = data.get("jobs")

    if not namespace:
        return jsonify({"error": "Missing 'namespace'"}), 400
    if not jobs or not isinstance(jobs, list):
        return jsonify({"error": "Missing or invalid 'jobs' list."}), 400

    try:
        if jobs == ["*"]:
            job_list = batch_v1.list_namespaced_job(namespace=namespace).items
            job_names = [job.metadata.name for job in job_list]
        else:
            job_names = jobs

        results = delete_job_list(job_names, namespace)
        return jsonify({"deleted": results}), 200

    except ApiException as e:
        return (
            jsonify({"error": "Failed to process job deletion.", "details": e.body}),
            e.status,
        )


def delete_job_list(job_names: list, namespace: str):
    results = []
    for job_name in job_names:
        try:
            batch_v1.delete_namespaced_job(
                name=job_name,
                namespace=namespace,
                body=client.V1DeleteOptions(propagation_policy="Foreground"),
            )
            results.append({"job": job_name, "status": "deleted"})
        except ApiException as e:
            results.append({"job": job_name, "status": "failed", "error": e.body})
    return results


@jobs_bp.route("/jobs", methods=["GET"])
def list_jobs():
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        job_list = batch_v1.list_namespaced_job(namespace=namespace)
        jobs = []
        for job in job_list.items:
            jobs.append(
                {
                    "name": job.metadata.name,
                    "start_time": (
                        str(job.status.start_time) if job.status.start_time else None
                    ),
                    "completion_time": (
                        str(job.status.completion_time)
                        if job.status.completion_time
                        else None
                    ),
                    "active": job.status.active or 0,
                    "succeeded": job.status.succeeded or 0,
                    "failed": job.status.failed or 0,
                }
            )

        return jsonify({"jobs": jobs}), 200

    except ApiException as e:
        return jsonify({"error": "Failed to list jobs", "details": e.body}), e.status


@jobs_bp.route("/jobs/create", methods=["POST"])
def generate_jobs():
    try:
        data = request.get_json()
        scenarios = data.get("scenarios")
        namespace = data.get("namespace", "default")

        if not scenarios or not isinstance(scenarios, list):
            return jsonify({"error": "Missing or invalid 'scenarios' list"}), 400

        result = generate_jobs_from_scenario_names(scenarios, namespace)
        return jsonify(result), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500


@jobs_bp.route("/jobs/status", methods=["GET"])
def get_job_status():
    namespace = request.args.get("namespace")
    job_name = request.args.get("job_name")

    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        if not job_name:
            job_list = batch_v1.list_namespaced_job(namespace=namespace)
            statuses = []

            for job in job_list.items:
                status = parse_job_status(job.status)
                statuses.append(
                    {"job": job.metadata.name, "namespace": namespace, "status": status}
                )

            return jsonify({"jobs": statuses}), 200
        else:
            job = batch_v1.read_namespaced_job(name=job_name, namespace=namespace)
            status = parse_job_status(job.status)
            return jsonify(
                {"job": job.metadata.name, "namespace": namespace, "status": status}
            )
    except ApiException as e:
        return (
            jsonify(
                {
                    "error": f"Job '{job_name}' not found in namespace '{namespace}'",
                    "details": e.body,
                }
            ),
            e.status,
        )


def parse_job_status(status_obj):
    if status_obj.succeeded:
        return "Completed"
    elif status_obj.failed:
        return "Failed"
    elif status_obj.active:
        return "Running"
    else:
        return "Unknown"

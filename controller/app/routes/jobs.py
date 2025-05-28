from flask import Blueprint, request, jsonify
from kubernetes import client
from kubernetes.client.rest import ApiException
from app.utils.kube_utils import load_kube_config

# Load Kubernetes configuration and initialize Job API
load_kube_config()
batch_v1 = client.BatchV1Api()

jobs_bp = Blueprint("jobs", __name__)


@jobs_bp.route("/jobs/create", methods=["POST"])
def create_jobs():
    """
    Create Kubernetes Jobs from a list of scenario file names.

    JSON Payload:
    {
        "scenarios": ["scenario1.yaml", "scenario2.yaml"],
        "namespace": "optional-namespace"
    }
    """
    data = request.get_json()
    scenarios = data.get("scenarios")
    namespace = data.get("namespace", "default")

    if not scenarios or not isinstance(scenarios, list):
        return jsonify({"error": "Missing or invalid 'scenarios' list"}), 400

    try:
        result = generate_jobs_from_scenario_names(scenarios, namespace)
        return jsonify(result), 200
    except Exception as e:
        return jsonify({"error": "Internal server error", "details": str(e)}), 500


@jobs_bp.route("/jobs", methods=["GET"])
def list_jobs():
    """
    List all jobs in the specified namespace with basic status information.

    Query Parameters:
    - namespace: required
    """
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        job_list = batch_v1.list_namespaced_job(namespace)
        jobs = [
            {
                "name": job.metadata.name,
                "start_time": (
                    job.status.start_time.isoformat() if job.status.start_time else None
                ),
                "completion_time": (
                    job.status.completion_time.isoformat()
                    if job.status.completion_time
                    else None
                ),
                "active": job.status.active or 0,
                "succeeded": job.status.succeeded or 0,
                "failed": job.status.failed or 0,
            }
            for job in job_list.items
        ]
        return jsonify({"jobs": jobs}), 200

    except ApiException as e:
        return jsonify({"error": "Failed to list jobs", "details": e.body}), e.status


@jobs_bp.route("/jobs/status", methods=["GET"])
def get_job_status():
    """
    Get status of a specific job or all jobs in a namespace.

    Query Parameters:
    - namespace: required
    - job_name: optional (if omitted, returns all job statuses)
    """
    namespace = request.args.get("namespace")
    job_name = request.args.get("job_name")

    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        if job_name:
            job = batch_v1.read_namespaced_job(name=job_name, namespace=namespace)
            status = parse_job_status(job.status)
            return (
                jsonify(
                    {"job": job.metadata.name, "namespace": namespace, "status": status}
                ),
                200,
            )
        else:
            jobs = batch_v1.list_namespaced_job(namespace).items
            statuses = [
                {
                    "job": job.metadata.name,
                    "namespace": namespace,
                    "status": parse_job_status(job.status),
                }
                for job in jobs
            ]
            return jsonify({"jobs": statuses}), 200

    except ApiException as e:
        return (
            jsonify(
                {
                    "error": (
                        f"Job '{job_name}' not found in namespace '{namespace}'"
                        if job_name
                        else "Failed to fetch job status"
                    ),
                    "details": e.body,
                }
            ),
            e.status,
        )


@jobs_bp.route("/jobs/delete", methods=["DELETE"])
def delete_jobs():
    """
    Delete one or more jobs in the specified namespace.

    JSON Payload:
    {
        "namespace": "example-ns",
        "jobs": ["job1", "job2"] or ["*"]  # "*" deletes all jobs
    }
    """
    data = request.get_json()
    namespace = data.get("namespace")
    jobs = data.get("jobs")

    if not namespace:
        return jsonify({"error": "Missing 'namespace'"}), 400
    if not jobs or not isinstance(jobs, list):
        return jsonify({"error": "Missing or invalid 'jobs' list"}), 400

    try:
        if jobs == ["*"]:
            job_list = batch_v1.list_namespaced_job(namespace).items
            job_names = [job.metadata.name for job in job_list]
        else:
            job_names = jobs

        results = delete_job_list(job_names, namespace)
        return jsonify({"deleted": results}), 200

    except ApiException as e:
        return jsonify({"error": "Failed to delete jobs", "details": e.body}), e.status


def delete_job_list(job_names: list, namespace: str):
    """
    Helper function to delete multiple jobs with foreground propagation policy.

    Args:
        job_names (list): List of job names to delete
        namespace (str): Kubernetes namespace

    Returns:
        list: List of job deletion results
    """
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


def parse_job_status(status_obj):
    """
    Parses Kubernetes JobStatus object into a human-readable status.

    Args:
        status_obj (V1JobStatus): Kubernetes JobStatus object

    Returns:
        str: "Completed", "Failed", "Running", or "Unknown"
    """
    if status_obj.succeeded:
        return "Completed"
    elif status_obj.failed:
        return "Failed"
    elif status_obj.active:
        return "Running"
    return "Unknown"

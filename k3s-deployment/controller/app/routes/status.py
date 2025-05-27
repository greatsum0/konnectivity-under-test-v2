from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException

status_bp = Blueprint("status", __name__)

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

batch_v1 = client.BatchV1Api()


@status_bp.route("/status/<job_name>", methods=["GET"])
def get_job_status(job_name):
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
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


@status_bp.route("/status", methods=["GET"])
def get_all_job_statuses():
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        job_list = batch_v1.list_namespaced_job(namespace=namespace)
        statuses = []

        for job in job_list.items:
            status = parse_job_status(job.status)
            statuses.append(
                {"job": job.metadata.name, "namespace": namespace, "status": status}
            )

        return jsonify({"jobs": statuses}), 200

    except ApiException as e:
        return jsonify({"error": "Failed to get jobs", "details": e.body}), e.status


def parse_job_status(status_obj):
    if status_obj.succeeded:
        return "Completed"
    elif status_obj.failed:
        return "Failed"
    elif status_obj.active:
        return "Running"
    else:
        return "Unknown"

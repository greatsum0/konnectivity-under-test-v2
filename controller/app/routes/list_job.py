from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException

list_jobs_bp = Blueprint("list_jobs", __name__)

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

batch_v1 = client.BatchV1Api()


@list_jobs_bp.route("/jobs/list", methods=["GET"])
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

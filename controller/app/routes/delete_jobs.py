from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException

delete_jobs_bp = Blueprint("delete_jobs", __name__)

# Kube config
try:
    config.load_kube_config()
except:
    config.load_incluster_config()

batch_v1 = client.BatchV1Api()

@delete_jobs_bp.route("/jobs/delete", methods=["DELETE"])
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
        return jsonify({
            "error": "Failed to process job deletion.",
            "details": e.body
        }), e.status


def delete_job_list(job_names: list, namespace: str):
    results = []
    for job_name in job_names:
        try:
            batch_v1.delete_namespaced_job(
                name=job_name,
                namespace=namespace,
                body=client.V1DeleteOptions(propagation_policy="Foreground")
            )
            results.append({"job": job_name, "status": "deleted"})
        except ApiException as e:
            results.append({"job": job_name, "status": "failed", "error": e.body})
    return results

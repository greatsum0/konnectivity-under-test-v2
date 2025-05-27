from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException

delete_manifests_bp = Blueprint("delete_manifests", __name__)

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

batch_v1 = client.BatchV1Api()


@delete_manifests_bp.route("/manifests/delete", methods=["DELETE"])
def delete_manifests():
    data = request.get_json()
    m_type = data.get("type")
    namespace = data.get("namespace")
    jobs = data.get("jobs")

    if not namespace:
        return jsonify({"error": "Missing 'namespace'"}), 400

    if m_type == "all":
        return delete_all_jobs(namespace)

    if m_type != "jobs":
        return jsonify({"error": "Invalid type. Must be 'jobs' or 'all'."}), 400
    if not jobs or not isinstance(jobs, list):
        return jsonify({"error": "Missing or invalid 'jobs' list."}), 400

    results = delete_job_list(jobs, namespace)
    return jsonify({"deleted": results}), 200


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


def delete_all_jobs(namespace: str):
    try:
        jobs = batch_v1.list_namespaced_job(namespace=namespace).items
        job_names = [job.metadata.name for job in jobs]
        results = delete_job_list(job_names, namespace)
        return jsonify({"deleted": results}), 200
    except ApiException as e:
        return (
            jsonify(
                {
                    "error": f"Failed to list jobs in namespace '{namespace}'",
                    "details": e.body,
                }
            ),
            e.status,
        )

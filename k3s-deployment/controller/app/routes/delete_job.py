from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException

delete_bp = Blueprint("delete", __name__)

try:
    config.load_incluster_config()
except config.ConfigException:
    config.load_kube_config()


@delete_bp.route("/jobs/<job_name>", methods=["DELETE"])
def delete_job(job_name):
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    try:
        batch_v1 = client.BatchV1Api()

        delete_opts = client.V1DeleteOptions(propagation_policy="Foreground")
        response = batch_v1.delete_namespaced_job(
            name=job_name, namespace=namespace, body=delete_opts
        )

        return (
            jsonify(
                {
                    "status": "deleted",
                    "job": job_name,
                    "namespace": namespace,
                    "details": str(response.status),
                }
            ),
            200,
        )

    except ApiException as e:
        return jsonify(
            {
                "error": f"Failed to delete job '{job_name}' in namespace '{namespace}'",
                "details": e.body,
            }
        ), (e.status if e.status else 500)

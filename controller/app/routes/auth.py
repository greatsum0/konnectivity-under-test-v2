from flask import Blueprint, request, jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException
import base64
import json

auth_bp = Blueprint("auth", __name__)

try:
    config.load_kube_config()
except:
    config.load_incluster_config()

core_v1 = client.CoreV1Api()


@auth_bp.route("/auth/docker", methods=["POST"])
def docker_auth():
    data = request.get_json()
    username = data.get("username")
    token = data.get("token")
    namespace = data.get("namespace")
    regcred = data.get("regcred", "kut-regcred")

    if not username or not token or not namespace or not regcred:
        return (
            jsonify(
                {
                    "error": "Missing required fields: username, token, namespace or regcred"
                }
            ),
            400,
        )

    try:
        try:
            core_v1.read_namespace(name=namespace)
        except ApiException as e:
            if e.status == 404:
                ns_body = client.V1Namespace(
                    metadata=client.V1ObjectMeta(name=namespace)
                )
                core_v1.create_namespace(body=ns_body)
            else:
                raise e

        auth_data = {
            "auths": {
                "https://index.docker.io/v1/": {
                    "username": username,
                    "password": token,
                    "auth": base64.b64encode(f"{username}:{token}".encode()).decode(),
                }
            }
        }
        docker_config_json = base64.b64encode(json.dumps(auth_data).encode()).decode()

        secret = client.V1Secret(
            metadata=client.V1ObjectMeta(name=regcred),
            type="kubernetes.io/dockerconfigjson",
            data={".dockerconfigjson": docker_config_json},
        )

        try:
            core_v1.read_namespaced_secret(name=regcred, namespace=namespace)
            core_v1.replace_namespaced_secret(
                name=regcred, namespace=namespace, body=secret
            )
        except ApiException as e:
            if e.status == 404:
                core_v1.create_namespaced_secret(namespace=namespace, body=secret)
            else:
                raise e

        return (
            jsonify({"status": "success", "secret": regcred, "namespace": namespace}),
            200,
        )

    except ApiException as e:
        return (
            jsonify({"error": f"Kubernetes API error: {e.reason}", "details": e.body}),
            e.status,
        )
    except Exception as e:
        return jsonify({"error": f"Exception occurred: {str(e)}"}), 500

from flask import Blueprint, request, jsonify
from kubernetes import client
from kubernetes.client.rest import ApiException
import base64
import json
from app.utils.kube_utils import load_kube_config  # Utility to load kube config properly

auth_bp = Blueprint("auth", __name__)

# Load Kubernetes config (from local or in-cluster)
load_kube_config()
core_v1 = client.CoreV1Api()


@auth_bp.route("/auth/docker", methods=["POST"])
def docker_auth():
    """
    Creates or replaces a Docker registry secret in a specified namespace.

    Expects JSON payload:
    {
        "username": "...",
        "token": "...",
        "namespace": "...",
        "regcred": "..." (optional, default: "kut-regcred")
    }
    """
    data = request.get_json()
    username = data.get("username")
    token = data.get("token")
    namespace = data.get("namespace")
    regcred = data.get("regcred", "kut-regcred")  # Default secret name

    if not all([username, token, namespace]):
        return (
            jsonify({"error": "Missing required fields: username, token, namespace"}),
            400,
        )

    try:
        # Ensure namespace exists
        try:
            core_v1.read_namespace(namespace)
        except ApiException as e:
            if e.status == 404:
                core_v1.create_namespace(
                    client.V1Namespace(metadata=client.V1ObjectMeta(name=namespace))
                )
            else:
                raise

        # Prepare Docker config data encoded in base64
        auth_str = f"{username}:{token}"
        docker_auth = base64.b64encode(auth_str.encode()).decode()

        auth_data = {
            "auths": {
                "https://index.docker.io/v1/": {
                    "username": username,
                    "password": token,
                    "auth": docker_auth,
                }
            }
        }

        docker_config_b64 = base64.b64encode(json.dumps(auth_data).encode()).decode()

        secret = client.V1Secret(
            metadata=client.V1ObjectMeta(name=regcred),
            type="kubernetes.io/dockerconfigjson",
            data={".dockerconfigjson": docker_config_b64},
        )

        # Create or replace the secret
        try:
            core_v1.read_namespaced_secret(regcred, namespace)
            core_v1.replace_namespaced_secret(regcred, namespace, secret)
            action = "replaced"
        except ApiException as e:
            if e.status == 404:
                core_v1.create_namespaced_secret(namespace, secret)
                action = "created"
            else:
                raise

        return (
            jsonify(
                {
                    "status": "success",
                    "action": action,
                    "secret": regcred,
                    "namespace": namespace,
                }
            ),
            200,
        )

    except ApiException as e:
        return jsonify({"error": "Kubernetes API error", "details": e.body}), e.status
    except Exception as e:
        return jsonify({"error": "Internal server error", "details": str(e)}), 500

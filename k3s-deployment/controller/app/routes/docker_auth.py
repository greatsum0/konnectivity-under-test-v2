from flask import Blueprint, request, jsonify
import subprocess

docker_auth_bp = Blueprint("docker_auth", __name__)


@docker_auth_bp.route("/auth/docker", methods=["POST"])
def docker_auth():
    data = request.get_json()
    username = data.get("username")
    token = data.get("token")
    namespace = data.get("namespace")
    regcred = data.get("regcred", "kut-regcred")

    if not username or not token or not namespace or not regcred:
        return (
            jsonify({"error": "Missing required fields: username, token, namespace or regcred"}),
            400,
        )

    try:
        # Namespace oluştur (yoksa)
        create_ns = subprocess.run(
            [
                "kubectl",
                "create",
                "namespace",
                namespace,
                "--dry-run=client",
                "-o",
                "yaml",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        subprocess.run(
            ["kubectl", "apply", "-f", "-"], input=create_ns.stdout, text=True
        )

        # Docker secret oluştur veya güncelle
        create_secret = subprocess.run(
            [
                "kubectl",
                "create",
                "secret",
                "docker-registry",
                regcred,
                f"--docker-username={username}",
                f"--docker-password={token}",
                "-n",
                namespace,
                "--dry-run=client",
                "-o",
                "yaml",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        subprocess.run(
            ["kubectl", "apply", "-f", "-"], input=create_secret.stdout, text=True
        )

        return (
            jsonify(
                {"status": "success", "secret": regcred, "namespace": namespace}
            ),
            200,
        )

    except Exception as e:
        return jsonify({"error": f"Exception occurred: {str(e)}"}), 500

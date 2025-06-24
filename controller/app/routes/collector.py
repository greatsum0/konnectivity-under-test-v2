from flask import Blueprint, request, jsonify, send_file
from kubernetes.stream import stream
from kubernetes import client as k8s_client
from app.utils.kube_utils import load_kube_config
import zipfile
import io
import tarfile
import tempfile

collection_bp = Blueprint("collection", __name__)

@collection_bp.route("/collect/zip-api", methods=["POST"])
def collect_zip_via_api():
    # query: ?namespace=xyz
    namespace = request.args.get("namespace", "kut-app")

    # JSON: { "client_pod": "xxx", "client_path": "/measurements/client", ... }
    data = request.get_json()
    client_pod = data.get("client_pod")
    client_path = data.get("client_path")
    server_pod = data.get("server_pod")
    server_path = data.get("server_path")

    if not all([client_pod, client_path, server_pod, server_path]):
        return jsonify({"status": "error", "message": "Missing fields in body"}), 400

    load_kube_config()
    core_v1 = k8s_client.CoreV1Api()

    zip_buffer = io.BytesIO()

    with zipfile.ZipFile(zip_buffer, "w", zipfile.ZIP_DEFLATED) as zipf:

        def copy_from_pod(pod_name, src_path, label):
            exec_command = ['tar', 'cf', '-', '-C', src_path, '.']
            try:
                resp = stream(
                    core_v1.connect_get_namespaced_pod_exec,
                    pod_name,
                    namespace,
                    command=exec_command,
                    stderr=True,
                    stdin=False,
                    stdout=True,
                    tty=False,
                    _preload_content=False
                )

                # tar stream -> extract -> add to zip
                with tempfile.TemporaryFile() as tar_stream:
                    while resp.is_open():
                        resp.update(timeout=1)
                        if resp.peek_stdout():
                            tar_stream.write(resp.read_stdout().encode('utf-8', errors='ignore'))
                        if resp.peek_stderr():
                            _ = resp.read_stderr()
                    resp.close()

                    tar_stream.seek(0)
                    with tarfile.open(fileobj=tar_stream) as tar:
                        for member in tar.getmembers():
                            if member.isfile():
                                file_data = tar.extractfile(member).read()
                                arcname = f"{label}/{member.name}"
                                zipf.writestr(arcname, file_data)
            except Exception as e:
                raise RuntimeError(f"Error copying from pod {pod_name}: {e}")

        try:
            copy_from_pod(client_pod, client_path, "client")
            copy_from_pod(server_pod, server_path, "server")
        except Exception as e:
            return jsonify({"status": "error", "message": str(e)}), 500

    zip_buffer.seek(0)
    return send_file(
        zip_buffer,
        mimetype="application/zip",
        as_attachment=True,
        download_name=f"{namespace}_results.zip"
    )

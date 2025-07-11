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
def collect_zip_from_viewers():
    namespace = request.args.get("namespace", "kut-app")

    load_kube_config()
    core_v1 = k8s_client.CoreV1Api()

    # Find all pods with label app=viewer
    try:
        pods = core_v1.list_namespaced_pod(namespace, label_selector="app=viewer").items
        if not pods:
            return jsonify({"status": "error", "message": "No viewer pods found"}), 404
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

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
            for pod in pods:
                copy_from_pod(pod.metadata.name, "/mnt", pod.metadata.name)
        except Exception as e:
            return jsonify({"status": "error", "message": str(e)}), 500

    zip_buffer.seek(0)
    return send_file(
        zip_buffer,
        mimetype="application/zip",
        as_attachment=True,
        download_name=f"{namespace}_viewer_results.zip"
    )

from flask import Blueprint, jsonify, request
from kubernetes import client, config
from kubernetes.client.rest import ApiException

cluster_status_bp = Blueprint("cluster_status", __name__)

@cluster_status_bp.route("/cluster/status", methods=["GET"])
def cluster_status():
    namespace = request.args.get("namespace")
    if not namespace:
        return jsonify({"error": "Missing 'namespace' query parameter"}), 400

    log_lines = []

    def log(line=""):
        log_lines.append(line)

    def header(title):
        log("")
        log(f"--- {title} ---")

    def print_resource_list(title, items, extract_func):
        header(title)
        if not items:
            log("No resources found.")
        else:
            for item in items:
                log(extract_func(item))

    try:
        config.load_kube_config()
    except Exception as e:
        return jsonify({"error": f"Failed to load Kubernetes config: {str(e)}"}), 500

    log(f"Cluster status for namespace: {namespace}")

    try:
        v1 = client.CoreV1Api()
        batch = client.BatchV1Api()
        apps = client.AppsV1Api()
        networking = client.NetworkingV1Api()

        print_resource_list("Nodes", v1.list_node().items, lambda n: n.metadata.name)
        print_resource_list("Deployments", apps.list_namespaced_deployment(namespace).items, lambda d: d.metadata.name)
        print_resource_list("Jobs", batch.list_namespaced_job(namespace).items, lambda j: f"{j.metadata.name} (status: {'Completed' if j.status.succeeded else 'Running'})")
        print_resource_list("Pods", v1.list_namespaced_pod(namespace).items, lambda p: f"{p.metadata.name} (phase: {p.status.phase})")
        print_resource_list("PVCs", v1.list_namespaced_persistent_volume_claim(namespace).items, lambda pvc: f"{pvc.metadata.name} (status: {pvc.status.phase})")
        print_resource_list("PVs", v1.list_persistent_volume().items, lambda pv: f"{pv.metadata.name} (status: {pv.status.phase})")
        print_resource_list("Secrets", v1.list_namespaced_secret(namespace).items, lambda s: s.metadata.name)
        print_resource_list("ConfigMaps", v1.list_namespaced_config_map(namespace).items, lambda cm: cm.metadata.name)
        print_resource_list("Services", v1.list_namespaced_service(namespace).items, lambda svc: svc.metadata.name)

        try:
            print_resource_list("Ingresses", networking.list_namespaced_ingress(namespace).items, lambda i: i.metadata.name)
        except ApiException as e:
            if e.status == 404:
                log("Ingress resource not available.")
            else:
                log(f"Failed to list ingresses: {e}")

        header("Events (last 10)")
        events = v1.list_namespaced_event(namespace).items
        events = sorted(events, key=lambda e: e.last_timestamp or e.event_time or e.metadata.creation_timestamp, reverse=True)
        for event in events[:10]:
            log(f"{event.last_timestamp} - {event.involved_object.kind}/{event.involved_object.name} - {event.message}")

        return jsonify({
            "namespace": namespace,
            "log": "\n".join(log_lines)
        }), 200

    except ApiException as e:
        return jsonify({"error": f"Kubernetes API error: {str(e)}"}), 500

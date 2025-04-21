from flask import jsonify
from kubernetes import client, config
from kubernetes.client.rest import ApiException
from env_loader import load_environment, get_env
from logger import CustomLogger
from pathlib import Path

# Singleton logger initialization
logger = CustomLogger("status", Path(__file__).parent.parent / "system.log")

def cluster_status():
    load_environment()

    def log_header(title):
        logger.info("")
        logger.info(f"--- {title} ---")

    def print_resource_list(title, items, extract_func):
        log_header(title)
        if not items:
            logger.info("No resources found.")
        else:
            for item in items:
                logger.info(extract_func(item))

    # Load Kubernetes config
    try:
        config.load_kube_config()
    except Exception as e:
        logger.error(f"Failed to load Kubernetes config: {e}")
        return jsonify({"error": "Kubernetes config load error"}), 500

    namespace = get_env("NAMESPACE", "kut-app")
    logger.info(f"Cluster status for namespace '{namespace}'")

    try:
        v1 = client.CoreV1Api()
        batch = client.BatchV1Api()
        apps = client.AppsV1Api()
        networking = client.NetworkingV1Api()

        # Resource sections
        print_resource_list("Nodes", v1.list_node().items, lambda n: n.metadata.name)
        print_resource_list("Deployments", apps.list_namespaced_deployment(namespace).items,
                            lambda d: d.metadata.name)
        print_resource_list("Jobs", batch.list_namespaced_job(namespace).items,
                            lambda j: f"{j.metadata.name} (status: {j.status.conditions or 'Running'})")
        print_resource_list("Pods", v1.list_namespaced_pod(namespace).items,
                            lambda p: f"{p.metadata.name} (phase: {p.status.phase})")
        print_resource_list("PVCs", v1.list_namespaced_persistent_volume_claim(namespace).items,
                            lambda pvc: f"{pvc.metadata.name} (status: {pvc.status.phase})")
        print_resource_list("PVs", v1.list_persistent_volume().items,
                            lambda pv: f"{pv.metadata.name} (status: {pv.status.phase})")
        print_resource_list("Secrets", v1.list_namespaced_secret(namespace).items,
                            lambda s: s.metadata.name)
        print_resource_list("ConfigMaps", v1.list_namespaced_config_map(namespace).items,
                            lambda cm: cm.metadata.name)
        print_resource_list("Services", v1.list_namespaced_service(namespace).items,
                            lambda svc: svc.metadata.name)

        # Ingresses
        try:
            print_resource_list("Ingresses", networking.list_namespaced_ingress(namespace).items,
                                lambda i: i.metadata.name)
        except ApiException as e:
            if e.status == 404:
                logger.warning("Ingress resource not available in this cluster.")
            else:
                logger.error(f"Failed to list ingresses: {e}")

        # Events
        log_header("Events (last 10)")
        events = v1.list_namespaced_event(namespace).items
        events = sorted(events, key=lambda e: e.last_timestamp or e.event_time or e.metadata.creation_timestamp,
                        reverse=True)
        for event in events[:10]:
            logger.info(f"{event.last_timestamp} - {event.involved_object.kind}/{event.involved_object.name} - {event.message}")

        return jsonify({"status": "status checked"}), 200

    except ApiException as e:
        logger.error(f"Kubernetes API error: {e}")
        return jsonify({"error": "Kubernetes API error"}), 500

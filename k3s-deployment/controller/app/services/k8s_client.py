from kubernetes import client, config
from kubernetes import utils
import yaml

def list_pods(namespace: str = None):
    config.load_kube_config()  # Use load_incluster_config() inside cluster
    v1 = client.CoreV1Api()

    if namespace:
        pods = v1.list_namespaced_pod(namespace=namespace, watch=False)
    else:
        pods = v1.list_pod_for_all_namespaces(watch=False)

    return [
        {
            "name": pod.metadata.name,
            "namespace": pod.metadata.namespace,
            "status": pod.status.phase,
            "node": pod.spec.node_name
        }
        for pod in pods.items
    ]

def apply_manifest(yaml_manifest: str):
    config.load_kube_config()
    k8s_client = client.ApiClient()

    # manifest birden fazla obje içerebilir
    documents = list(yaml.safe_load_all(yaml_manifest))
    for doc in documents:
        utils.create_from_dict(k8s_client, data=doc)
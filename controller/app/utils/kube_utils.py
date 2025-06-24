from kubernetes import config


def load_kube_config():
    """Loads kube config, falling back to in-cluster config."""
    try:
        config.load_kube_config()
    except:
        config.load_incluster_config()

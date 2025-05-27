from flask import Flask

def create_app():
    app = Flask(__name__)

    # Blueprint register
    from .routes.pods import pods_bp
    app.register_blueprint(pods_bp)
    from .routes.generate_configmap import configmap_bp
    app.register_blueprint(configmap_bp)
    from .routes.generate_jobs import jobs_bp
    app.register_blueprint(jobs_bp)
    from .routes.settings import settings_bp
    app.register_blueprint(settings_bp)
    from .routes.apply_manifests import apply_manifest_bp
    app.register_blueprint(apply_manifest_bp)
    from .routes.namespaces import namespaces_bp
    app.register_blueprint(namespaces_bp)
    from .routes.status import status_bp
    app.register_blueprint(status_bp)
    from .routes.pod_logs import logs_bp
    app.register_blueprint(logs_bp)
    from .routes.cluster_status import cluster_status_bp
    app.register_blueprint(cluster_status_bp)
    from .routes.docker_auth import docker_auth_bp
    app.register_blueprint(docker_auth_bp)
    from .routes.delete_jobs import delete_jobs_bp
    app.register_blueprint(delete_jobs_bp)
    from .routes.cluster_reset import cluster_reset_bp
    app.register_blueprint(cluster_reset_bp)
    return app

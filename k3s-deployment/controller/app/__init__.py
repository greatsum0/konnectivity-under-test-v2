from flask import Flask

def create_app():
    app = Flask(__name__)

    # Blueprint register
    from .routes.pods import pods_bp
    app.register_blueprint(pods_bp)
    from .routes.configmap import configmap_bp
    app.register_blueprint(configmap_bp)
    from .routes.jobs import jobs_bp
    app.register_blueprint(jobs_bp)
    from .routes.settings import settings_bp
    app.register_blueprint(settings_bp)
    from .routes.manifests import manifests_bp
    app.register_blueprint(manifests_bp)
    from .routes.namespaces import namespaces_bp
    app.register_blueprint(namespaces_bp)
    from .routes.status import status_bp
    app.register_blueprint(status_bp)
    from .routes.logs import logs_bp
    app.register_blueprint(logs_bp)
    from .routes.delete_job import delete_bp
    app.register_blueprint(delete_bp)
    from .routes.cluster_status import cluster_status_bp
    app.register_blueprint(cluster_status_bp)
    from .routes.docker_auth import docker_auth_bp
    app.register_blueprint(docker_auth_bp)
    from .routes.delete_manifests import delete_manifests_bp
    app.register_blueprint(delete_manifests_bp)
    return app

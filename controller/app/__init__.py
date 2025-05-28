from flask import Flask
from .routes.jobs import jobs_bp
from .routes.configmap import configmap_bp
from .routes.settings import settings_bp
from .routes.namespaces import namespace_bp
from .routes.pods import pods_bp
from .routes.auth import auth_bp
from .routes.scenarios import scenarios_bp

from .routes.cluster import cluster_bp
from .routes.apply_manifests import (
    apply_configmap_bp,
    apply_job_bp,
    apply_pvc_bp,
    apply_deployment_bp,
    apply_service_bp,
)
from .routes.pod_logs import logs_bp
from .routes.generate_service import services_bp
from .routes.generate_deployment import deployments_bp
from .routes.generate_viewer_pods import viewer_pods_bp


def create_app():
    app = Flask(__name__)

    app.register_blueprint(jobs_bp)
    app.register_blueprint(configmap_bp)
    app.register_blueprint(settings_bp)
    app.register_blueprint(namespace_bp)
    app.register_blueprint(pods_bp)
    app.register_blueprint(auth_bp)
    app.register_blueprint(scenarios_bp)

    app.register_blueprint(cluster_bp)
    app.register_blueprint(apply_configmap_bp)
    app.register_blueprint(apply_job_bp)
    app.register_blueprint(apply_pvc_bp)
    app.register_blueprint(apply_deployment_bp)
    app.register_blueprint(apply_service_bp)
    app.register_blueprint(logs_bp)
    app.register_blueprint(services_bp)
    app.register_blueprint(deployments_bp)
    app.register_blueprint(viewer_pods_bp)
    return app

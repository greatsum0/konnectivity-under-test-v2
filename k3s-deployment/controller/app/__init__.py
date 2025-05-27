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
    return app

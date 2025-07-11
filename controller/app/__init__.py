from flask import Flask
from .routes.jobs import jobs_bp
from .routes.namespaces import namespace_bp
from .routes.pods import pods_bp
from .routes.auth import auth_bp
from .routes.scenarios import scenarios_bp
from .routes.cluster import cluster_bp
from .routes.collector import collection_bp
from .routes.render import render_bp
from .routes.apply import apply_bp

def create_app():
    app = Flask(__name__)

    app.register_blueprint(jobs_bp)
    app.register_blueprint(namespace_bp)
    app.register_blueprint(pods_bp)
    app.register_blueprint(auth_bp)
    app.register_blueprint(scenarios_bp)
    app.register_blueprint(cluster_bp)
    app.register_blueprint(collection_bp)
    app.register_blueprint(render_bp)
    app.register_blueprint(apply_bp)
    return app

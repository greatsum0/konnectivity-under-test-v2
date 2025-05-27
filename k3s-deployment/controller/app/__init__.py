from flask import Flask

def create_app():
    app = Flask(__name__)

    # Blueprint register
    from .routes.pods import pods_bp
    app.register_blueprint(pods_bp)
    from .routes.configmap import configmap_bp
    app.register_blueprint(configmap_bp)

    return app

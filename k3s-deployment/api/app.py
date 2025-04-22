from flask import Flask, jsonify
from flasgger import Swagger
from endpoints.reset import perform_reset
from endpoints.run_all import run_all_jobs
from endpoints.status import cluster_status
from endpoints.trigger import trigger_scenarios
from endpoints.base_deployment import base_deployment
from endpoints.copy_results import copy_results
from endpoints.download_results import download_results

app = Flask(__name__)
swagger = Swagger(app)

# Register endpoints
app.add_url_rule("/status", view_func=cluster_status, methods=["GET"])
app.add_url_rule("/reset", view_func=perform_reset, methods=["POST"])
app.add_url_rule("/run-all", view_func=run_all_jobs, methods=["POST"])
app.add_url_rule("/base-deployment", view_func=base_deployment, methods=["POST"])
app.add_url_rule("/trigger-scenarios", view_func=trigger_scenarios, methods=["POST"])
app.add_url_rule("/copy-results", view_func=copy_results, methods=["POST"])
app.add_url_rule("/download-results", view_func=download_results, methods=["GET"])

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5050)

from flask import Blueprint, jsonify, request
from kubernetes import client, config, utils
from kubernetes.client.rest import ApiException
from app.utils.kube_utils import load_kube_config
import yaml
import os

# Blueprint definition
apply_bp = Blueprint("apply", __name__)

# Load Kubernetes configuration once
load_kube_config()
k8s_client = client.ApiClient()

def apply_manifest_file(filename: str) -> dict:
    """
    Applies a Kubernetes manifest file using the Kubernetes API.

    Args:
        filename (str): Path to the YAML file to apply

    Returns:
        dict: Success message or error details
    """
    if not filename.endswith((".yaml", ".yml")):
        raise ValueError("Only .yaml or .yml files are allowed")

    if not os.path.exists(filename):
        raise FileNotFoundError(f"YAML file not found: {filename}")

    try:
        with open(filename, "r") as f:
            yaml_objects = list(yaml.safe_load_all(f))

        applied = []
        for obj in yaml_objects:
            resp = utils.create_from_dict(k8s_client, obj)
            if isinstance(resp, list):
                applied.extend([r.to_dict() for r in resp])
            else:
                applied.append(resp.to_dict())

        return {"success": True, "applied_objects": applied}

    except ApiException as e:
        return {
            "success": False,
            "error": f"Kubernetes API error: {e.reason}",
            "details": e.body,
        }

    except yaml.YAMLError as e:
        return {"success": False, "error": "YAML parsing error", "details": str(e)}

    except Exception as e:
        return {"success": False, "error": "Internal error", "details": str(e)}


@apply_bp.route("/apply", methods=["POST"])
def apply_yaml():
    """
    POST /apply
    Applies the provided YAML file using the Kubernetes Python client.

    JSON Body:
        - filename (str): Path to the YAML file

    Returns:
        - Success or error message
    """
    data = request.get_json()
    if not data or "filename" not in data:
        return jsonify({"error": "Missing 'filename' in request body"}), 400

    try:
        result = apply_manifest_file(data["filename"])
        status_code = 200 if result["success"] else 500
        return jsonify(result), status_code

    except FileNotFoundError as e:
        return jsonify({"error": str(e)}), 404

    except ValueError as e:
        return jsonify({"error": str(e)}), 400

    except Exception as e:
        return jsonify({"error": "Internal server error", "details": str(e)}), 500

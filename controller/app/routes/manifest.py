from flask import Blueprint, request, jsonify
from kubernetes import client, utils
from kubernetes.client.rest import ApiException
import yaml

from app.services.manifest_utils import (
    load_template,
    write_rendered_yaml,
    validate_yaml,
)
from app.utils.kube_utils import load_kube_config
from app.utils.dir_utils import get_dir

# Load Kubernetes configuration and create API client
load_kube_config()
k8s_client = client.ApiClient()

# Flask Blueprint for manifest operations
manifest_bp = Blueprint("manifest", __name__)


@manifest_bp.route("/manifests/render/<template_name>", methods=["POST"])
def render_manifest(template_name):
    """
    Render a Jinja2 manifest template with provided JSON values.

    Path Parameters:
        - template_name (str): Template file name without extension

    JSON Payload:
    {
        "name": "my-deployment",
        ... template variables ...
    }

    Returns:
        JSON response containing:
        - status
        - rendered YAML content
        - output file path and name
    """
    try:
        values = request.get_json()
        if not values:
            return jsonify({"error": "Missing or invalid JSON body"}), 400

        name = values.get("name", "default")

        # Load and render the Jinja2 template
        template = load_template(template_name)
        rendered_yaml = template.render(**values)

        # Validate the rendered YAML content
        try:
            validate_yaml(rendered_yaml)
        except ValueError as e:
            return jsonify({"error": f"YAML validation failed: {str(e)}"}), 400

        # Write rendered YAML to file
        output_path = write_rendered_yaml(name, rendered_yaml)

        return (
            jsonify(
                {
                    "status": "success",
                    "filename": output_path.name,
                    "output_path": str(output_path),
                    "rendered": rendered_yaml,
                }
            ),
            200,
        )

    except FileNotFoundError as e:
        return jsonify({"error": f"Template not found: {str(e)}"}), 404
    except Exception as e:
        return jsonify({"error": "Failed to render manifest", "details": str(e)}), 500


@manifest_bp.route("/manifests/apply", methods=["POST"])
def apply_manifest():
    """
    Apply a previously rendered YAML manifest to the Kubernetes cluster.

    JSON Payload:
    {
        "filename": "my-deployment-manifest.yaml"
    }

    Returns:
        JSON response showing resource creation/replacement status.
    """
    try:
        data = request.get_json()
        filename = data.get("filename")
        if not filename:
            return jsonify({"error": "Missing 'filename' in request body"}), 400

        manifest_dir = get_dir("manifests", ensure=True)
        manifest_path = manifest_dir / filename

        if not manifest_path.exists():
            return jsonify({"error": f"Manifest file '{filename}' not found"}), 404

        with open(manifest_path, "r") as f:
            documents = list(yaml.safe_load_all(f))

        applied = []
        for doc in documents:
            kind = doc.get("kind")
            name = doc.get("metadata", {}).get("name")

            try:
                utils.create_from_dict(k8s_client, doc)
                applied.append({"kind": kind, "name": name, "status": "created"})
            except ApiException as e:
                if e.status == 409:
                    # Replace existing resource
                    utils.create_from_dict(k8s_client, doc, force=True)
                    applied.append({"kind": kind, "name": name, "status": "replaced"})
                else:
                    applied.append(
                        {
                            "kind": kind,
                            "name": name,
                            "status": "failed",
                            "error": e.body,
                        }
                    )

        return jsonify({"filename": filename, "applied": applied}), 200

    except Exception as e:
        return jsonify({"error": "Failed to apply manifest", "details": str(e)}), 500

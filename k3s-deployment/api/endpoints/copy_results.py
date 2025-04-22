from flask import jsonify
import subprocess
from pathlib import Path
from logger import CustomLogger
from env_loader import get_env
from flasgger import swag_from

logger = CustomLogger("copy_results", Path(__file__).parent.parent / "system.log")

@swag_from({
    'tags': ['Scenarios'],
    'summary': 'Trigger scenario jobs based on changed files',
    'description': 'Triggers specific jobs based on the provided list of changed scenario files.',
    'consumes': ['application/json'],
    'produces': ['application/json'],
    'parameters': [
        {
            'in': 'body',
            'name': 'body',
            'required': True,
            'schema': {
                'type': 'object',
                'properties': {
                    'message': {
                        'type': 'string',
                        'example': 'Test'
                    },
                    'changed_files': {
                        'type': 'array',
                        'items': {
                            'type': 'string',
                            'example': 'burst.yaml'
                        }
                    }
                },
                'required': ['message', 'changed_files']
            }
        }
    ],
    'responses': {
        200: {
            'description': 'Trigger successful',
            'schema': {
                'type': 'object',
                'properties': {
                    'status': {
                        'type': 'string',
                        'example': 'triggered'
                    }
                }
            }
        },
        400: {
            'description': 'Invalid or missing JSON payload',
            'schema': {
                'type': 'object',
                'properties': {
                    'error': {
                        'type': 'string',
                        'example': 'No JSON received'
                    }
                }
            }
        }
    }
})
def copy_results():
    namespace = get_env("NAMESPACE", "kut-app")
    dest_dir = Path.home() / "kut-data"
    client_dir = dest_dir / "client"
    server_dir = dest_dir / "server"

    client_dir.mkdir(parents=True, exist_ok=True)
    server_dir.mkdir(parents=True, exist_ok=True)

    client_pod = "pvc-viewer-client"

    try:
        # Get server pod name
        result = subprocess.run(
            ["kubectl", "get", "pods", "-n", namespace, "-l", "app=kut-server",
             "-o", "jsonpath={.items[0].metadata.name}"],
            capture_output=True, text=True, check=True
        )
        server_pod = result.stdout.strip()
        logger.info(f"Server pod resolved: {server_pod}")
    except subprocess.CalledProcessError as e:
        logger.error("Failed to get server pod name.")
        return jsonify({"status": "error", "message": str(e), "step": "get-server-pod"}), 500

    try:
        subprocess.run(
            ["kubectl", "cp", f"{namespace}/{client_pod}:/measurements/data", str(client_dir)],
            check=True
        )
        logger.info(f"Client data copied to {client_dir}")
    except subprocess.CalledProcessError as e:
        logger.error("Failed to copy data from client pod.")
        return jsonify({"status": "error", "message": str(e), "step": "copy-client"}), 500

    try:
        subprocess.run(
            ["kubectl", "cp", f"{namespace}/{server_pod}:/measurements/data", str(server_dir)],
            check=True
        )
        logger.info(f"Server data copied to {server_dir}")
    except subprocess.CalledProcessError as e:
        logger.error("Failed to copy data from server pod.")
        return jsonify({"status": "error", "message": str(e), "step": "copy-server"}), 500

    logger.info("All results copied successfully.")
    return jsonify({"status": "copied"}), 200

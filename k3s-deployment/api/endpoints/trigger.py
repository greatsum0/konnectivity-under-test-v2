from flask import request, jsonify
from flasgger import swag_from
from logger import CustomLogger
from pathlib import Path
from endpoints.run_changed import run_changed_jobs

logger = CustomLogger("trigger", Path(__file__).parent.parent / "system.log")

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
def trigger_scenarios():
    data = request.get_json()
    if not data:
        return jsonify({"error": "No JSON received"}), 400

    logger.info("Webhook received.")
    run_changed_jobs(data)
    return jsonify({"status": "triggered"}), 200

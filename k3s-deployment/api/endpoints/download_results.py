from flask import send_file, jsonify
from flasgger import swag_from
from pathlib import Path
import shutil
import tempfile
from logger import CustomLogger

logger = CustomLogger("download_results", Path(__file__).parent.parent / "system.log")

@swag_from({
    'tags': ['Results'],
    'summary': 'Download measurement results as ZIP',
    'description': 'Compresses and returns all copied client/server result files as a downloadable ZIP file.',
    'produces': ['application/zip'],
    'responses': {
        200: {
            'description': 'ZIP file containing all results',
            'content': {
                'application/zip': {
                    'schema': {
                        'type': 'string',
                        'format': 'binary'
                    }
                }
            }
        },
        404: {
            'description': 'Results directory not found',
            'schema': {
                'type': 'object',
                'properties': {
                    'error': {
                        'type': 'string',
                        'example': 'Results directory not found'
                    }
                }
            }
        },
        500: {
            'description': 'Server error while zipping or sending the file',
            'schema': {
                'type': 'object',
                'properties': {
                    'error': {
                        'type': 'string',
                        'example': 'Failed to create ZIP archive'
                    }
                }
            }
        }
    }
})
def download_results():
    base_dir = Path("/home/oguz/kut-data")
    if not base_dir.exists():
        return jsonify({"error": "Results directory not found"}), 404

    try:
        with tempfile.NamedTemporaryFile(suffix=".zip", delete=False) as tmpfile:
            zip_path = Path(tmpfile.name)

        shutil.make_archive(str(zip_path).replace('.zip', ''), 'zip', base_dir)

        logger.info(f"Zipped results at {zip_path}")
        return send_file(
            zip_path,
            as_attachment=True,
            download_name="kut-results.zip",
            mimetype="application/zip"
        )

    except Exception as e:
        logger.error(f"Failed to zip and send results: {e}")
        return jsonify({"error": str(e)}), 500

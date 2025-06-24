from pathlib import Path
from jinja2 import Template
import yaml

from app.utils.dir_utils import get_dir


def load_template(template_name: str) -> Template:
    """
    Load a Jinja2 template by name from the templates directory.

    Args:
        template_name (str): Name of the template file (without .yaml extension)

    Returns:
        Template: Jinja2 Template object

    Raises:
        FileNotFoundError: If the template file does not exist
    """
    template_dir = get_dir("templates")
    template_path = template_dir / f"{template_name}.yaml"

    if not template_path.exists():
        raise FileNotFoundError(f"Template '{template_name}' not found.")

    return Template(template_path.read_text())


def write_rendered_yaml(name: str, rendered_yaml: str) -> Path:
    """
    Write rendered YAML content to a file under the manifests directory.

    Args:
        name (str): Output base filename
        rendered_yaml (str): Rendered YAML string

    Returns:
        Path: Path to the written YAML file
    """
    output_dir = get_dir("manifests")
    output_path = output_dir / f"{name}-manifest.yaml"
    output_path.write_text(rendered_yaml)
    return output_path


def validate_yaml(yaml_content: str):
    """
    Validate the syntax of a rendered YAML string.

    Args:
        yaml_content (str): The YAML content to validate

    Raises:
        ValueError: If the YAML is syntactically invalid
    """
    try:
        yaml.safe_load(yaml_content)
    except yaml.YAMLError as e:
        raise ValueError(f"Rendered YAML is invalid: {str(e)}")

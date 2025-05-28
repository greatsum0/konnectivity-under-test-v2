from pathlib import Path


def get_project_root() -> Path:
    """Returns the root path of the project."""
    return Path(__file__).resolve().parent.parent.parent


def get_dir(dir_type: str, ensure: bool = True) -> Path:
    """
    Returns a directory path for the given type. Optionally creates it if not exists.

    Args:
        dir_type (str): Type of directory ('scenarios', 'templates', 'manifests', ...)
        ensure (bool): Whether to create the directory if missing (default: True)

    Returns:
        Path: Path to the requested directory

    Raises:
        ValueError: If an unsupported dir_type is provided
    """
    root = get_project_root()
    mapping = {
        "scenarios": root / "scenarios",
        "templates": root / "templates",
        "manifests": root / "manifests",
        "configmaps": root / "configmaps",
    }

    if dir_type not in mapping:
        raise ValueError(
            f"Unsupported dir_type '{dir_type}'. Supported types: {list(mapping.keys())}"
        )

    path = mapping[dir_type]

    if ensure:
        path.mkdir(parents=True, exist_ok=True)

    return path


def dir_exists(dir_type: str) -> bool:
    """
    Checks whether the directory for the given type exists.

    Args:
        dir_type (str): The type of directory to check

    Returns:
        bool: True if directory exists, False otherwise

    Raises:
        ValueError: If an unsupported dir_type is provided
    """
    return get_dir(dir_type, ensure=False).exists()

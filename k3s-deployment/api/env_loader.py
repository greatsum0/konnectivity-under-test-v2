from pathlib import Path
from dotenv import load_dotenv
import os
from logger import CustomLogger

logger = CustomLogger("env_loader", Path(__file__).parent / "system.log")

def load_environment(env_path=None):
    """
    Loads environment variables from a .env file.
    Defaults to a .env file in the same directory as this script.
    """
    if env_path is None:
        env_path = Path(__file__).resolve().parent / ".env"

    if not env_path.exists():
        logger.warning(f".env file not found at {env_path}")
    else:
        load_dotenv(dotenv_path=env_path, override=True)
        logger.info(f"Environment loaded from {env_path}")

def get_env(key: str, default=None):
    """
    Fetches an environment variable with optional default fallback.
    """
    return os.getenv(key, default)

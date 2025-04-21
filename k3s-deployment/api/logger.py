import logging
import sys
from pathlib import Path

class CustomLogger:
    _instances = {}

    def __new__(cls, name: str, log_file: Path):
        if name in cls._instances:
            return cls._instances[name]

        instance = super().__new__(cls)
        cls._instances[name] = instance
        return instance

    def __init__(self, name: str, log_file: Path):
        if hasattr(self, 'initialized'):
            return  # Prevent reinitialization

        self.logger = logging.getLogger(name)
        self.logger.setLevel(logging.DEBUG)

        formatter = logging.Formatter(
            f"[%(levelname)s][%(asctime)s][{name}] %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S"
        )

        if not self.logger.handlers:
            console_handler = logging.StreamHandler(sys.stdout)
            console_handler.setFormatter(formatter)

            file_handler = logging.FileHandler(log_file)
            file_handler.setFormatter(formatter)

            self.logger.addHandler(console_handler)
            self.logger.addHandler(file_handler)

        self.initialized = True

    def info(self, msg): self.logger.info(msg)
    def warning(self, msg): self.logger.warning(msg)
    def error(self, msg): self.logger.error(msg)
    def debug(self, msg): self.logger.debug(msg)

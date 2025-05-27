import yaml
from pathlib import Path

SETTINGS_FILE = Path(__file__).resolve().parent.parent.parent / "settings.yaml"

DEFAULTS = {
    "DOCKER_AUTH_USERNAME": "",
    "DOCKER_AUTH_TOKEN": "",
    "IMAGE_NAME": "kut/client",
    "IMAGE_TAG": "latest"
}

class SettingsManager:
    def __init__(self):
        self.settings = DEFAULTS.copy()
        self._load()

    def _load(self):
        if SETTINGS_FILE.exists():
            with open(SETTINGS_FILE) as f:
                data = yaml.safe_load(f)
                if data:
                    self.settings.update(data)

    def save(self):
        with open(SETTINGS_FILE, "w") as f:
            yaml.dump(self.settings, f)

    def update(self, new_settings: dict):
        self.settings.update({k: v for k, v in new_settings.items() if k in DEFAULTS})
        self.save()

    def get(self):
        return self.settings.copy()

# Global instance
settings_manager = SettingsManager()

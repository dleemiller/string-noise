import json
import importlib.resources
from ..string_noise import augment_string


class Mapper:

    def __init__(self, json_data):
        self._json_data = json_data
        if not self._validate():
            raise ValueError("Invalid JSON structure")

    @property
    def data(self):
        return self._json_data

    @staticmethod
    def _is_valid_key(key):
        return isinstance(key, str)

    @staticmethod
    def _is_valid_value(value):
        return isinstance(value, (str, list, dict))

    def _validate(self):
        if not isinstance(self._json_data, dict):
            return False

        for key, value in self._json_data.items():
            if not self._is_valid_key(key) or not self._is_valid_value(value):
                return False

        return True

    @classmethod
    def load(cls, resource_package, resource_path):
        with importlib.resources.open_text(resource_package, resource_path) as file:
            json_data = json.load(file)
            return cls(json_data)

    def __call__(self, text: str, probability: float):
        return augment_string(text, self.data, probability, debug=False)

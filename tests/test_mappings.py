import unittest
import tempfile
import io
import json
from string_noise.mappings import Mapper
from unittest.mock import patch


class TestMapper(unittest.TestCase):
    @patch("importlib.resources.open_text")
    def test_list_values_json(self, mock_open):
        json_data = {"list1": ["item1", "item2"], "list2": ["item3", "item4"]}
        json_data_norm = {
            "list1": {"item1": 0.5, "item2": 0.5},
            "list2": {"item3": 0.5, "item4": 0.5},
        }
        mock_open.return_value.__enter__.return_value = io.StringIO(
            json.dumps(json_data)
        )
        loader = Mapper.load("dummy_package", "dummy_path")
        self.assertEqual(loader.data, json_data_norm)

    @patch("importlib.resources.open_text")
    def test_dict_values_json(self, mock_open):
        json_data = {"dict1": {"k1": 2, "k2": 8}, "dict2": {"k1": 3, "k2": 8}}
        json_data_norm = {
            "dict1": {"k1": 2 / 10, "k2": 8 / 10},
            "dict2": {"k1": 3 / 11, "k2": 8 / 11},
        }
        mock_open.return_value.__enter__.return_value = io.StringIO(
            json.dumps(json_data)
        )
        loader = Mapper.load("dummy_package", "dummy_path")
        self.assertEqual(loader.data, json_data_norm)

    @patch("importlib.resources.open_text")
    def test_invalid_json(self, mock_open):
        invalid_json_data = "This is not a JSON object"
        mock_open.return_value.__enter__.return_value = io.StringIO(invalid_json_data)
        with self.assertRaises(json.decoder.JSONDecodeError):
            Mapper.load("dummy_package", "dummy_path")


if __name__ == "__main__":
    unittest.main()

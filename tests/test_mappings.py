import unittest
import tempfile
import json
from string_noise.mappings import MappingLoader

class TestMappingLoader(unittest.TestCase):

    def create_temp_json_file(self, data):
        temp_file = tempfile.NamedTemporaryFile(mode='w+', delete=False, suffix='.json')
        json.dump(data, temp_file)
        temp_file.close()
        return temp_file.name

    def test_string_values_json(self):
        json_data = {"key1": "value", "key2": "another value"}
        temp_file = self.create_temp_json_file(json_data)
        loader = MappingLoader.load(temp_file)
        self.assertEqual(loader.data, json_data)

    def test_list_values_json(self):
        json_data = {"list1": ["item1", "item2"], "list2": ["item3", "item4"]}
        temp_file = self.create_temp_json_file(json_data)
        loader = MappingLoader.load(temp_file)
        self.assertEqual(loader.data, json_data)

    def test_dict_values_json(self):
        json_data = {"dict1": {"key": "value"}, "dict2": {"another key": "another value"}}
        temp_file = self.create_temp_json_file(json_data)
        loader = MappingLoader.load(temp_file)
        self.assertEqual(loader.data, json_data)

    def test_invalid_json(self):
        json_data = "This is not a JSON object"
        temp_file = self.create_temp_json_file(json_data)
        with self.assertRaises(ValueError):
            MappingLoader.load(temp_file)

# Run the tests
if __name__ == '__main__':
    unittest.main()


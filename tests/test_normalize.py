import unittest
from string_noise import string_noise


class TestNormalizeFunction(unittest.TestCase):
    def test_normalize_nested_dict(self):
        input_dict = {"a": {"x": 0.5, "y": 0.5}, "b": {"p": 0.3, "q": 0.7}}
        expected_output = {"a": {"x": 0.5, "y": 0.5}, "b": {"p": 0.3, "q": 0.7}}
        normalized_dict = string_noise.normalize(input_dict)
        self.assertEqual(normalized_dict, expected_output)

    def test_normalize_dict_with_list(self):
        input_dict = {"a": ["x", "y", "z"]}
        expected_output = {"a": {"x": 1 / 3, "y": 1 / 3, "z": 1 / 3}}
        normalized_dict = string_noise.normalize(input_dict)
        self.assertEqual(normalized_dict, expected_output)

    def test_normalize_simple_mapping(self):
        input_dict = {"a": "b"}
        expected_output = {"a": {"b": 1.0}}
        normalized_dict = string_noise.normalize(input_dict)
        self.assertEqual(normalized_dict, expected_output)

    def test_normalize_empty_dict(self):
        input_dict = {}
        expected_output = {}
        normalized_dict = string_noise.normalize(input_dict)
        self.assertEqual(normalized_dict, expected_output)

    def test_invalid_input_type(self):
        with self.assertRaises(TypeError):
            string_noise.normalize(["not", "a", "dict"])

    def test_normalize_mixed_types(self):
        input_dict = {"a": {"x": 0.2, "y": 0.8}, "b": ["u", "v", "w"], "c": "d"}
        expected_output = {
            "a": {"x": 0.2, "y": 0.8},
            "b": {"u": 1 / 3, "v": 1 / 3, "w": 1 / 3},
            "c": {"d": 1.0},
        }
        normalized_dict = string_noise.normalize(input_dict)
        self.assertEqual(normalized_dict, expected_output)

    def test_normalize_weights_greater_than_one(self):
        input_dict = {"a": {"a": 2.0, "b": 3.0, "c": 5.0}}
        expected_output = {"a": {"a": 2.0 / 10.0, "b": 3.0 / 10.0, "c": 5.0 / 10.0}}
        normalized_dict = string_noise.normalize(input_dict)
        self.assertEqual(normalized_dict, expected_output)

        # Additionally, check if the sum of normalized values equals 1
        self.assertAlmostEqual(sum(normalized_dict["a"].values()), 1.0, places=5)


if __name__ == "__main__":
    unittest.main()

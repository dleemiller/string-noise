import random
import unittest
from string_noise import string_noise

class TestStringNoise(unittest.TestCase):
    def test_augmentation(self):
        mapping = {"a": ["e"], "e": ["i"], "o": ["u"]}
        input_string = "hello world"
        augmented_string = string_noise.augment_string(input_string, mapping, 1.0)
        self.assertEqual(augmented_string, "hillu wurld")

        input_string = "hello world"
        augmented_string = string_noise.augment_string(input_string, mapping, 0.0)
        self.assertEqual(input_string, augmented_string)  # Since probability is 0, it should never change

    def test_multi_character_sequences(self):
        mapping = {"abc": ["x"], "a": ["y"], "bc": ["z"]}
        input_string = "tabcdef"
        augmented_string = string_noise.augment_string(input_string, mapping, 1.0)
        self.assertEqual(augmented_string, "txdef")

    def test_single_replacement(self):
        mapping = {"o": ["u"], "oo": ["ue"]}
        input_string = "foot"
        # With probability 1, ensure only one replacement happens, preferring the longer match
        augmented_string = string_noise.augment_string(input_string, mapping, 1.0)
        self.assertEqual(augmented_string, "fuet")

    def test_probabilities(self):
        mapping = {"a": ["e"]}
        input_string = "banana"
        # With probability 0, the string should not change
        self.assertEqual(string_noise.augment_string(input_string, mapping, 0.0), "banana")
        # With probability 1, all 'a's should turn into 'e's
        self.assertEqual(string_noise.augment_string(input_string, mapping, 1.0), "benene")

    def test_invalid_inputs(self):
        mapping = {"a": ["e"]}
        # Invalid string input
        with self.assertRaises(TypeError):
            string_noise.augment_string(123, mapping, 1.0)
        # Invalid probability
        with self.assertRaises(ValueError):
            string_noise.augment_string("test", mapping, -0.1)
        with self.assertRaises(ValueError):
            string_noise.augment_string("test", mapping, 1.1)

    def test_different_data_types_in_mapping(self):
        mapping = {"a": "e", "b": ["c", "d"]}
        input_string = "abracadabra"
        # Mixed data types in mapping should still work with probability 1
        augmented_string = string_noise.augment_string(input_string, mapping, 1.0)
        # We don't know whether it will replace 'b' with 'c' or 'd', so check if either is true
        self.assertTrue(augmented_string.startswith("e") and "ec" in augmented_string or "ed" in augmented_string)

    def test_empty_string_input(self):
        mapping = {"a": ["e"]}
        self.assertEqual(string_noise.augment_string("", mapping, 1.0), "")

    def test_empty_mapping(self):
        input_string = "hello world"
        self.assertEqual(string_noise.augment_string(input_string, {}, 1.0), input_string)

    def test_replacement_with_empty_strings(self):
        mapping = {"a": [""], "b": [""]}
        self.assertEqual(string_noise.augment_string("abracadabra", mapping, 1.0, debug=True), "rcdr")

    def test_boundary_probabilities(self):
        mapping = {"a": ["e"]}
        input_string = "banana"
        # Not testing for exact output due to randomness, but function should not throw an error
        string_noise.augment_string(input_string, mapping, 0.01)
        string_noise.augment_string(input_string, mapping, 0.99)

    def test_non_ascii_characters(self):
        mapping = {"é": ["e"], "ö": ["o"]}
        self.assertEqual(string_noise.augment_string("café öl", mapping, 1.0), "cafe ol")

#    def test_overlapping_replacement_keys(self):
#        mapping = {"ab": ["x"], "bc": ["y"]}
#        self.assertEqual(string_noise.augment_string("abc", mapping, 1.0), "x")

    def test_replacement_list_with_multiple_items(self):
        random.seed(42)  # Fixed seed for reproducibility
        mapping = {"a": ["e", "i"]}
        self.assertEqual(string_noise.augment_string("banana", mapping, 1.0), "binini")

    def test_performance_large_strings(self):
        mapping = {"a": ["e"]}
        large_string = "a" * 10000
        # Just testing that it runs without error, not checking output
        string_noise.augment_string(large_string, mapping, 0.5)

    def test_invalid_types_in_replacement_lists(self):
        mapping = {"a": [1, "b"]}
        with self.assertRaises(TypeError):
            string_noise.augment_string("banana", mapping, 1.0)

    def test_repeated_characters_in_input_string(self):
        mapping = {"a": ["b"]}
        self.assertEqual(string_noise.augment_string("aaaa", mapping, 1.0), "bbbb")

    def test_mapping_with_non_string_keys(self):
        mapping = {1: ["a"]}
        with self.assertRaises(TypeError):
            string_noise.augment_string("1234", mapping, 1.0)

if __name__ == '__main__':
    unittest.main()

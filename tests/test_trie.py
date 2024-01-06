import unittest
from string_noise import string_noise
from string_noise.string_noise import MarkovTrie


class TestTrie(unittest.TestCase):
    def setUp(self):
        self.trie = string_noise.build_tree({})

    def test_insert_and_lookup(self):
        # Test insertion and lookup
        test_dict = {"hello": ["hola", "bonjour"], "world": ["mundo", "monde"]}
        self.trie = string_noise.build_tree(test_dict)

        # Check if words are correctly inserted
        for word in test_dict:
            self.assertIsNotNone(self.trie.lookup(word))

        # Check for a word not in the trie
        self.assertIsNone(self.trie.lookup("unknown"))

    def test_empty_trie(self):
        # Test empty trie
        self.assertIsNone(self.trie.lookup("anything"))

    def test_invalid_input(self):
        # Test handling of invalid input
        with self.assertRaises(TypeError):
            string_noise.build_tree("not a dict")

        # Test handling of non-string keys
        with self.assertRaises(TypeError):
            string_noise.build_tree({1: ["one"]})

    def test_memory_management(self):
        # Test memory management
        large_dict = {f"word{i}": [f"meaning{i}"] for i in range(1000)}
        large_trie = string_noise.build_tree(large_dict)
        for i in range(1000):
            self.assertIsNotNone(large_trie.lookup(f"word{i}"))

    def test_valid_inputs(self):
        # Test valid inputs
        valid_dict = {
            "word1": "single string",
            "word2": ["list", "of", "strings"],
            "word3": {"key1": 0.5, "key2": 0.3},
        }
        trie = string_noise.build_tree(valid_dict)
        for word in valid_dict:
            self.assertIsNotNone(trie.lookup(word))

    def test_invalid_input_types(self):
        # Test invalid dictionary input
        with self.assertRaises(TypeError):
            string_noise.build_tree("not a dict")

        # Test non-string keys
        with self.assertRaises(TypeError):
            string_noise.build_tree({1: ["one"]})

        # Test invalid value types
        with self.assertRaises(TypeError):
            string_noise.build_tree({"word": 123})  # Integer value
        with self.assertRaises(TypeError):
            string_noise.build_tree({"word": [1, 2, 3]})  # List of integers
        with self.assertRaises(TypeError):
            string_noise.build_tree(
                {"word": {"key": "not a float"}}
            )  # Dict with non-float value

    def test_mixed_valid_invalid_values(self):
        # Test mixed valid and invalid value types
        mixed_dict = {
            "valid1": "valid string",
            "invalid1": 123,
            "valid2": ["valid", "list"],
            "invalid2": [1, 2, 3],
        }
        with self.assertRaises(TypeError):
            string_noise.build_tree(mixed_dict)

    def test_empty_trie(self):
        # Test empty trie
        trie = string_noise.build_tree({})
        self.assertIsNone(trie.lookup("anything"))

    def test_index_string_basic(self):
        trie = MarkovTrie()
        count = trie.index_string("abcd")
        self.assertEqual(count, 2)  # "abc" should create 2 trigrams

    def test_index_string_edge_cases(self):
        trie = MarkovTrie()
        count = trie.index_string("")
        self.assertEqual(count, 0)  # Empty string

        count = trie.index_string("абв")  # Non-Latin characters
        self.assertEqual(count, 0)

    def test_dump_empty_trie(self):
        trie = MarkovTrie()
        dumped = trie.dump()
        self.assertEqual(dumped, {"forward": {}, "reverse": {}})

    def test_dump_populated_trie(self):
        trie = MarkovTrie()
        trie.index_string("abc")
        dumped = trie.dump()
        self.assertDictEqual(
            dumped,
            {"forward": {"a": {"b": {"c": 1}}}, "reverse": {"c": {"b": {"a": 1}}}},
        )

    def test_load_valid_data(self):
        trie = MarkovTrie()
        valid_data = {
            "forward": {"a": {"b": {"c": 1}}},
            "reverse": {"c": {"b": {"a": 1}}},
        }
        trie.load(valid_data)
        self.assertDictEqual(trie.dump(), valid_data)

    def test_load_invalid_data(self):
        trie = MarkovTrie()
        invalid_data = {"forward": "invalid", "reverse": "invalid"}
        with self.assertRaises(TypeError):
            trie.load(invalid_data)


if __name__ == "__main__":
    unittest.main()

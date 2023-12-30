import unittest
from string_noise.mispelling import random_mispelling
from string_noise.string_noise import build_tree

class TestRandomMispelling(unittest.TestCase):
    def setUp(self):
        # Set up the trie with test data
        test_data = {
            "This": ["Thiz"],
            "is": ["iz"],
            "a": ["a"],
            "test": ["tezt"],
            "Hello": ["Helo"],
            "World": ["Worldz"]
        }
        build_tree(test_data)

    def test_no_change(self):
        # Test case where no words should change (probability=0)
        text = "This is a test."
        modified_text = random_mispelling(text, probability=0, seed=42)
        self.assertEqual(text, modified_text)

    def test_all_change(self):
        # Test case where all words should change (probability=1)
        # Assuming your trie and lookup function are set up to return specific misspellings
        text = "This is a test."
        expected = "Thiz iz a tezt."  # Expected output based on your trie's misspellings
        modified_text = random_mispelling(text, probability=1, seed=42)
        self.assertEqual(expected, modified_text)

    def test_case_sensitivity(self):
        # Test to ensure case sensitivity is preserved
        text = "Hello World."
        modified_text = random_mispelling(text, probability=1, seed=42)
        self.assertTrue(modified_text[0].isupper() and modified_text[5].isupper())

    def test_punctuation_preservation(self):
        # Test to ensure punctuation is preserved
        text = "Hello, world!"
        modified_text = random_mispelling(text, probability=1, seed=42)
        self.assertEqual(modified_text[-1], '!')

    def test_randomness_with_seed(self):
        # Test to ensure that the seed parameter works as expected
        text = "Random test."
        modified_text1 = random_mispelling(text, probability=0.5, seed=42)
        modified_text2 = random_mispelling(text, probability=0.5, seed=42)
        self.assertEqual(modified_text1, modified_text2)

# Run the tests
if __name__ == '__main__':
    unittest.main()


import unittest
from string_noise.string_noise import (
    MarkovTrie,
    WHITESPACE_NONE,
    WHITESPACE_ZERO,
    WHITESPACE_BOUNDARY,
)


class TestMarkovTrieReplace(unittest.TestCase):
    def setUp(self):
        self.trie = MarkovTrie()
        self.trie.index_string("abc")  # Setup a basic trie

    def test_replace_with_high_probability(self):
        result = self.trie.replace("abg", probability=1.0)
        self.assertEqual(result, "abc", "Should replace characters")

    def test_replace_with_high_probability2(self):
        result = self.trie.replace("gbc", probability=1.0)
        self.assertEqual(result, "abc", "Should replace characters")

    def test_replace_with_low_probability(self):
        result = self.trie.replace("abg", probability=0.0)
        self.assertEqual(result, "abg", "Should not replace characters")

    def test_replace_empty_string(self):
        result = self.trie.replace("", probability=1.0)
        self.assertEqual(result, "", "Empty string should remain unchanged")

    def test_replace_non_ascii_characters(self):
        result = self.trie.replace("äöü", probability=1.0)
        self.assertEqual(result, "äöü", "Non-ASCII characters should remain unchanged")

    def test_replace_non_ascii_characters2(self):
        result = self.trie.replace("äöüabg", probability=1.0)
        self.assertEqual(
            result, "äöüabc", "Non-ASCII characters should remain unchanged"
        )

    def test_replace_long_string(self):
        long_string = "abg" * 100  # Repeat 'abcd' 100 times
        result = self.trie.replace(long_string, probability=1.0)
        self.assertEqual(
            result, "abc" * 100, "Should replace characters in long strings"
        )
        self.assertTrue(
            len(result) == len(long_string), "Length of result should match input"
        )

    def test_replace_whitespace(self):
        result = self.trie.replace("a b c", probability=1.0)
        self.assertEqual(
            result, "a b c", "Whitespace characters should remain unchanged"
        )

    def test_replace_with_different_trie_states(self):
        self.trie.index_string("def")
        result1 = self.trie.replace("def", probability=1.0)
        self.trie.index_string("ghi")
        result2 = self.trie.replace("def", probability=1.0)
        self.assertEqual(
            result1, result2, "Replacement varies with different trie states"
        )

    def test_replace_invalid_input(self):
        with self.assertRaises(TypeError):
            self.trie.replace(None, probability=1.0)  # Passing None as input
        with self.assertRaises(ValueError):
            self.trie.replace("abc", probability=-1.0)  # Invalid probability

    def test_replace_whitespace_none(self):
        # Assuming trie doesn't replace whitespace by default (WHITESPACE_NONE)
        result = self.trie.replace(
            "a b c", probability=1.0, zero_whitespace=WHITESPACE_NONE
        )
        self.assertEqual(
            result, "a b c", "Whitespace should remain unchanged with WHITESPACE_NONE"
        )

    def test_replace_whitespace_zero(self):
        self.trie.load({"forward": {"a": {"b": {" ": 1}}}, "reverse": {}})
        result = self.trie.replace(
            "abXc", probability=1.0, zero_whitespace=WHITESPACE_ZERO
        )
        expected = "abXc"
        self.assertEqual(result, expected)

    def test_replace_whitespace_boundary(self):
        self.trie.load({"forward": {"a": {"b": {" ": 1}}}, "reverse": {}})
        result = self.trie.replace(
            "abc", probability=1.0, zero_whitespace=WHITESPACE_BOUNDARY
        )
        self.assertEqual("ab ", result)

        self.trie.load({"reverse": {"e": {"d": {" ": 1}}}, "forward": {}})
        result = self.trie.replace(
            "cde", probability=1.0, zero_whitespace=WHITESPACE_BOUNDARY
        )
        self.assertEqual(" de", result)

        self.trie.load(
            {"reverse": {"e": {"d": {" ": 1}}}, "forward": {"a": {"b": {" ": 1}}}}
        )
        result = self.trie.replace(
            "abc cde", probability=1.0, zero_whitespace=WHITESPACE_BOUNDARY
        )
        self.assertEqual("ab   de", result)

    def test_replace_whitespace_boundary_no_change(self):
        self.trie.load(
            {"reverse": {"b": {"a": {" ": 1}}, " ": {"c": {" ": 1}}}, "forward": {}}
        )
        result = self.trie.replace(
            "aabc ", probability=1.0, zero_whitespace=WHITESPACE_BOUNDARY
        )
        self.assertEqual(
            result,
            " abc ",
            "Inner whitespace should remain unchanged with WHITESPACE_BOUNDARY",
        )


if __name__ == "__main__":
    unittest.main()

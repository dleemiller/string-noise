import unittest
from string_noise.string_noise import random_masking


class TestRandomMasking(unittest.TestCase):
    def test_empty_string(self):
        """Test masking on an empty string."""
        result = random_masking("", seed=123)
        self.assertEqual(result, "")

    def test_all_whitespace(self):
        """Test masking on a string of only whitespace."""
        result = random_masking("     ", seed=123)
        self.assertEqual(result, "     ")

    def test_no_masking(self):
        """Test with zero probability, no characters should be masked."""
        original = "abcdefg"
        result = random_masking(original, probability=0.0, seed=123)
        self.assertEqual(result, original)

    def test_full_masking(self):
        """Test with probability 1, all characters should be masked."""
        original = "abcdefg"
        result = random_masking(original, probability=1.0, seed=123)
        self.assertNotEqual(result, original)
        self.assertEqual(len(result), len(original))

    def test_known_seed(self):
        """Test with a known seed for predictable output."""
        original = "abcdefg"
        result = random_masking(original, seed=42, probability=0.5)
        expected = "\x06\tcd\t\x07\t"
        self.assertEqual(result, expected)

    def test_varying_lengths(self):
        """Test strings of varying lengths."""
        for length in range(1, 10):
            original = "a" * length
            result = random_masking(original, seed=123, probability=0.5)
            self.assertEqual(len(result), length)

    def test_control_characters(self):
        """Test custom control characters."""
        original = "abcde"
        result = random_masking(
            original,
            vowel_mask=0x02,
            consonant_mask=0x01,
            nws_mask=0x03,
            general_mask=0x04,
            probability=1.0,
            seed=13,
            general_mask_probability=0.2,
        )
        self.assertEqual(result, "\x02\x04\x04\x01\x04")

    def test_general_mask_probability(self):
        """Test general mask probability."""
        original = "abcde"
        result = random_masking(
            original, general_mask_probability=1.0, probability=1.0, seed=123
        )
        expected_result = "\x09\x09\x09\x09\x09"  # All characters should be masked with the general mask
        self.assertEqual(result, expected_result)

    def test_whitespace_and_characters(self):
        """Test strings with a mix of whitespace and characters."""
        original = "abc def ghi"
        result = random_masking(original, probability=0.5, seed=123)
        self.assertEqual(len(result), len(original))

    def test_boundary_conditions_for_consecutive(self):
        """Test boundary conditions for min_consecutive and max_consecutive."""
        original = "abc"
        result = random_masking(
            original, min_consecutive=4, max_consecutive=5, seed=123
        )
        self.assertEqual(result, original)  # No masking should occur

    def test_special_characters(self):
        """Test strings with special characters."""
        original = "!@#$%^&*()"
        result = random_masking(original, probability=1.0, seed=123)
        self.assertNotEqual(result, original)
        self.assertEqual(len(result), len(original))

    def test_unicode_characters(self):
        """Test strings with Unicode characters."""
        original = "你好世界"  # "Hello World" in Chinese
        result = random_masking(original, probability=0.5, seed=123)
        self.assertEqual(len(result), len(original))

    def test_very_long_string(self):
        """Test a very long string."""
        original = "a" * 10000
        result = random_masking(original, probability=0.5, seed=123)
        self.assertEqual(len(result), len(original))

    def test_general_mask_probability_edges(self):
        """Test edge cases for general_mask_probability."""
        original = "abcdef"
        # Test with general_mask_probability = 0
        result = random_masking(
            original, general_mask_probability=0, probability=1.0, seed=123
        )
        self.assertNotEqual(result, "\x09\x09\x09\x09\x09\x09")
        # Test with general_mask_probability = 0.5
        result = random_masking(
            original, general_mask_probability=0.5, probability=1.0, seed=123
        )
        self.assertNotEqual(result, original)
        self.assertEqual(len(result), len(original))

    def test_randomness(self):
        """Test the randomness without specifying a seed."""
        original = "abcdef"
        results = set(
            random_masking(original, probability=0.5, seed=-1) for _ in range(100)
        )
        self.assertTrue(len(results) > 1)  # Expect multiple different results


if __name__ == "__main__":
    unittest.main()

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
            vowel_mask=chr(2).encode(),
            consonant_mask=chr(1).encode(),
            nws_mask=chr(3).encode(),
            general_mask=chr(4).encode(),
            probability=1.0,
            seed=12,
            general_mask_probability=0.1,
        )
        self.assertEqual(result, "\x02\x01\x01\x01\x04")

    def test_general_mask_probability(self):
        """Test general mask probability."""
        original = "abcde"
        result = random_masking(
            original, general_mask_probability=1.0, probability=1.0, seed=123
        )
        expected_result = "\x09\x09\x09\x09\x09"  # All characters should be masked with the general mask
        self.assertEqual(result, expected_result)


if __name__ == "__main__":
    unittest.main()

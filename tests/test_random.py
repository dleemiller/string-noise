import unittest
from string_noise import string_noise


class RandomReplacementTests(unittest.TestCase):
    def test_basic_replacement(self):
        result = string_noise.random_replacement("hello", "xyz", seed=42, debug=False)
        expected = "xyello"
        self.assertEqual(result, expected)

    def test_no_replacement_with_zero_probability(self):
        result = string_noise.random_replacement("hello", "xyz", probability=0, seed=42)
        expected = "hello"
        self.assertEqual(result, expected)

    def test_full_replacement_with_full_probability(self):
        result = string_noise.random_replacement("hello", "xyz", probability=1, seed=42)
        self.assertEqual(result, "xyzxxx")

    def test_charset_limitation(self):
        result = string_noise.random_replacement("hello", "a", probability=1, seed=42)
        # Expect all characters to be 'a' since charset is only 'a'
        self.assertEqual(result, "aaaaaa")

    def test_length_variation(self):
        original = "yellow"
        result = string_noise.random_replacement(
            original,
            "xyz",
            min_chars_in=1,
            max_chars_in=1,
            min_chars_out=2,
            max_chars_out=2,
            probability=1.0,
            seed=42,
        )
        self.assertTrue(len(original) * 2 == len(result))

        result = string_noise.random_replacement(
            original,
            "xyz",
            min_chars_in=2,
            max_chars_in=2,
            min_chars_out=1,
            max_chars_out=1,
            probability=1.0,
            seed=42,
        )
        self.assertTrue(len(original) == len(result) * 2)

        result = string_noise.random_replacement(
            original,
            "xyz",
            min_chars_in=0,
            max_chars_in=0,
            min_chars_out=1,
            max_chars_out=1,
            probability=1.0,
            seed=42,
        )
        self.assertTrue(len(original) * 2 == len(result))

    def test_invalid_input(self):
        with self.assertRaises(ValueError):
            string_noise.random_replacement(
                "hello", ""
            )  # Empty charset should raise an error


if __name__ == "__main__":
    unittest.main()

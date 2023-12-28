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
            debug=False,
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
            debug=False,
        )
        self.assertTrue(len(original) * 2 == len(result))

    def test_invalid_input(self):
        with self.assertRaises(ValueError):
            string_noise.random_replacement(
                "hello", ""
            )  # Empty charset should raise an error

    def test_2byte_unicode_replacement(self):
        original = "hello\u00E9"  # '\u00E9' is a 2-byte character (é)
        charset = "\u00E9\u00F1"  # Including 2-byte characters (é, ñ)
        result = string_noise.random_replacement(
            original, charset, probability=1, seed=42
        )
        # The test checks that replacement occurs with 2-byte characters
        self.assertNotEqual(result, original)
        self.assertTrue(all(ch in charset for ch in result))

    def test_4byte_unicode_replacement(self):
        original = "hello\U0001F600"  # '\U0001F600' is a 4-byte character (😀)
        charset = "\U0001F601\U0001F602"  # Including 4-byte characters (😁, 😂)
        result = string_noise.random_replacement(
            original, charset, probability=1, seed=42
        )
        # The test checks that replacement occurs with 4-byte characters
        self.assertNotEqual(result, original)
        self.assertTrue(all(ch in charset or ch in original for ch in result))

    def test_mixed_byte_length_replacement(self):
        original = "hello\u00E9\U0001F600"  # Mixed 2-byte and 4-byte characters
        charset = "xyz\u00E9\U0001F602"  # Mixed 1-byte, 2-byte, and 4-byte characters
        result = string_noise.random_replacement(
            original, charset, probability=1, seed=42
        )
        # The test checks replacements with mixed byte-length characters
        self.assertNotEqual(result, original)
        self.assertTrue(all(ch in charset for ch in result))

    def test_random_replacement_invalid_input(self):
        # Test invalid input_string type
        with self.assertRaises(TypeError):
            string_noise.random_replacement(123, "abc")  # input_string should be a string

        # Test invalid charset type
        with self.assertRaises(TypeError):
            string_noise.random_replacement("hello", 123)  # charset should be a string

        # Test invalid probability range
        with self.assertRaises(ValueError):
            string_noise.random_replacement("hello", "abc", probability=-1)  # probability should be 0-1

        # Test invalid min_chars_in and max_chars_in values
        with self.assertRaises(ValueError):
            string_noise.random_replacement("hello", "abc", min_chars_in=-1)  # should be non-negative
        with self.assertRaises(ValueError):
            string_noise.random_replacement("hello", "abc", max_chars_in=0)  # should be at least 1


if __name__ == "__main__":
    unittest.main()

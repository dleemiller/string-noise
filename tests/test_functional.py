import unittest
from string_noise import noise


class TestLazyNoise(unittest.TestCase):
    def test_random_function(self):
        test_string = "Example text"
        result = noise.random(test_string, probability=1.0, seed=43)
        self.assertNotEqual(result, test_string)
        self.assertIn(" ", result)  # Ensuring whitespace is preserved

    def test_ocr_function(self):
        result = noise.ocr("Example text")
        self.assertIsNotNone(result)

    def test_basic_ocr_function(self):
        result = noise.basic_ocr("Example text")
        self.assertIsNotNone(result)

    def test_leet_function(self):
        result = noise.leet("Example text")
        self.assertIsNotNone(result)

    def test_keyboard_function(self):
        result = noise.keyboard("Example text")
        self.assertIsNotNone(result)

    def test_homoglyph_function(self):
        result = noise.homoglyph("Example text")
        self.assertIsNotNone(result)

    def test_phonetic_function(self):
        result = noise.phonetic("Example text")
        self.assertIsNotNone(result)


if __name__ == "__main__":
    unittest.main()

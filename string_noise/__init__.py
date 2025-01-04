from functools import cached_property
import string

from .mappings import *
from .noisers import (
    random_masking,
    random_mask_batch,
    random_replacement,
)
from .noisers import (
    DEFAULT_VOWEL_MASK,
    DEFAULT_CONSONANT_MASK,
    DEFAULT_DIGIT_MASK,
    DEFAULT_NWS_MASK,
    DEFAULT_GENERAL_MASK,
    DEFAULT_2BYTE_MASK,
    DEFAULT_4BYTE_MASK,
    SHUFFLE,
    RESHUFFLE,
    ASCENDING,
    DESCENDING,
)


class LazyNoise:
    """
    Implements various types of string noise augmentation including mapped and random noise.

    This class provides a suite of methods for augmenting strings by introducing noise.
    It includes predefined mappings for OCR-style noise, leet speak, keyboard errors, homoglyphs,
    and phonetic replacements, as well as a method for random character replacement.

    The class is inspired by the OCR noise analysis in the following paper:
    Nguyen, T.-T.-H., Jatowt, A., Coustaty, M., Nguyen, N.-V., & Doucet, A. (2020).
    Deep Statistical Analysis of OCR Errors for Effective Post-OCR Processing.
    In Proceedings of the 18th Joint Conference on Digital Libraries (pp. 29–38).
    IEEE Press. https://doi.org/10.1109/JCDL.2019.00015

    Available Methods:
    - random: Introduces random noise into the string.
    - ocr: Mimics OCR-related noise.
    - basic_ocr: A simplified version of OCR noise.
    - leet: Replaces characters with visually similar numbers or symbols (leet speak).
    - keyboard: Simulates typing errors based on keyboard layout.
    - homoglyph: Substitutes characters with visually similar glyphs.
    - phonetic: Applies phonetic-based character substitutions.

    Each method can be called directly from an instance of LazyNoise. For example:
    >>> from string_noise import noise, RESHUFFLE
    >>> noise.ocr("Example text", probability=0.9, sort_order=RESHUFFLE, seed=43)
    """

    @cached_property
    def ocr(self):
        """
        Loads and returns a mapping for OCR-style noise augmentation.
        """
        return load_nguyen_ocr()

    @cached_property
    def basic_ocr(self):
        """
        Loads and returns a basic mapping for OCR-style noise augmentation.
        """
        return load_llm_ocr()

    @cached_property
    def leet(self):
        """
        Loads and returns a mapping for leet speak-style noise augmentation.
        """
        return load_leet()

    @cached_property
    def keyboard(self):
        """
        Loads and returns a mapping for keyboard-style noise augmentation.
        """
        return load_keyboard()

    @cached_property
    def homoglyph(self):
        """
        Loads and returns a mapping for homoglyph-style noise augmentation.
        """
        return load_homoglyph()

    @cached_property
    def phonetic(self):
        """
        Loads and returns a mapping for phonetic-style noise augmentation.
        """
        return load_phonetic()

    @cached_property
    def mispelling(self):
        """
        Generates mispellings using C trie datastructure.
        """
        return load_mispelling()

    @cached_property
    def moe(self):
        """
        moe - Misspelling Oblivious Word Embeddings
        Generates MOE mispellings using C trie data structure.

        Pared down to top 167k words using top word counts from:
        https://norvig.com/ngrams/count_1w.txt

        See here for details:
        https://github.com/facebookresearch/moe
        """
        return load_moe_mispelling()

    @staticmethod
    def random(
        original_string: str,
        charset: str = string.ascii_letters + string.digits,
        min_chars_in: int = 1,
        max_chars_in: int = 2,
        min_chars_out: int = 1,
        max_chars_out: int = 2,
        probability: float = 0.1,
        seed: int = -1,
        debug: bool = False,
    ):
        """
        Randomly replaces characters in a string with characters from a given charset.

        Parameters:
        - original_string (str): The string to which the random replacements are to be applied.
        - charset (str, optional): Characters to use as replacements. Defaults to ascii letters and digits.
        - min_chars_in (int, optional): Minimum number of consecutive characters to be considered for replacement. Defaults to 1.
        - max_chars_in (int, optional): Maximum number of consecutive characters to be considered for replacement. Defaults to 2.
        - min_chars_out (int, optional): Minimum number of replacement characters. Defaults to 1.
        - max_chars_out (int, optional): Maximum number of replacement characters. Defaults to 2.
        - probability (float, optional): Probability of any character being replaced (0-1). Defaults to 0.1.
        - seed (int, optional): Seed for the random number generator, -1 for random. Defaults to -1.
        - debug (bool, optional): Enables debug output if set to True. Defaults to False.

        Returns:
        - (str): A new string with random replacements applied.

        Raises:
        - ValueError: If input parameters are invalid.

        Examples:
        >>> noise.random(original_string="hello world", charset="abc", probability=0.5, seed=42)
        'abcaaa woacd'
        >>> noise.random(original_string="test", charset="xyz", probability=1, seed=123)
        'xxyy'
        """
        return random_replacement(
            original_string,
            charset,
            min_chars_in=min_chars_in,
            max_chars_in=max_chars_in,
            min_chars_out=min_chars_out,
            max_chars_out=max_chars_out,
            probability=probability,
            seed=seed,
            debug=debug,
        )

    @staticmethod
    def mask(
        text: str | list[str],
        probability: float = 0.1,
        min_consecutive: int = 1,
        max_consecutive: int = 1,
        vowel_mask: int = DEFAULT_VOWEL_MASK,
        consonant_mask: int = DEFAULT_CONSONANT_MASK,
        digit_mask: int = DEFAULT_DIGIT_MASK,
        nws_mask: int = DEFAULT_NWS_MASK,
        general_mask: int = DEFAULT_GENERAL_MASK,
        two_byte_mask: int = DEFAULT_2BYTE_MASK,
        four_byte_mask: int = DEFAULT_4BYTE_MASK,
        general_mask_probability: float = 0.5,
        seed: int = -1,
        skip_digits: bool = False,
        debug: bool = False,
    ):
        """
        Applies random masking to a string based on character properties and Unicode byte size.

        Parameters:
        - text (str | list[str]): The string(s) to which the masking is to be applied.
        - probability (float, optional): Probability of each character being masked (0-1). Defaults to 0.1.
        - min_consecutive (int, optional): Minimum number of consecutive characters to consider for masking. Defaults to 1.
        - max_consecutive (int, optional): Maximum number of consecutive characters to consider for masking. Defaults to 2.
        - vowel_mask (int, optional): Mask value for vowels. Defaults to 0x06.
        - consonant_mask (int, optional): Mask value for consonants. Defaults to 0x07.
        - nws_mask (int, optional): Mask value for non-whitespace characters. Defaults to 0x08.
        - general_mask (int, optional): General mask value for characters. Defaults to 0x09.
        - two_byte_mask (int, optional): Mask value for 2-byte Unicode characters. Defaults to 0x0A.
        - four_byte_mask (int, optional): Mask value for 4-byte Unicode characters. Defaults to 0x0B.
        - general_mask_probability (float, optional): Probability of using the general mask instead of specific masks (0-1). Defaults to 0.5.
        - seed (int, optional): Seed for the random number generator, -1 for random. Defaults to -1.
        - debug (bool, optional): Enables debug output if set to True. Defaults to False.

        Returns:
        - (str): A new string with random masking applied.

        Raises:
        - ValueError: If input parameters are invalid.

        Examples:
        >>> noise.mask("Hello world!", probability=0.5, seed=42)
        'H\x06ll\x09 w\x07rld!'
        >>> noise.mask("Python", vowel_mask=0x01, consonant_mask=0x02, probability=1, seed=123)
        '\x02\x01th\x02n'
        """
        if isinstance(text, str):
            mask_fn = random_masking
        elif isinstance(text, list):
            mask_fn = random_mask_batch
        else:
            raise TypeError("Input text must be `str` or `list[str]`")

        return mask_fn(
            text=text,
            probability=probability,
            min_consecutive=min_consecutive,
            max_consecutive=max_consecutive,
            vowel_mask=vowel_mask,
            consonant_mask=consonant_mask,
            nws_mask=nws_mask,
            general_mask=general_mask,
            two_byte_mask=two_byte_mask,
            four_byte_mask=four_byte_mask,
            general_mask_probability=general_mask_probability,
            seed=seed,
            skip_digits=int(skip_digits),
            debug=int(debug),
        )


noise = LazyNoise()

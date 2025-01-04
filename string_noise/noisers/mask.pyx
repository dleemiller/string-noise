# mask.pyx
# cython: language_level=3
# cython: boundscheck=False
# cython: wraparound=False

from cpython.unicode cimport (
    Py_UNICODE_ISSPACE, 
    Py_UNICODE_ISDECIMAL, 
    Py_UNICODE_ISALPHA, 
    Py_UNICODE_TOLOWER
)

from cython.view cimport array as cvarray
import random


# ---------------------------------------------------------------------
# Default masks
cdef int DEFAULT_VOWEL_MASK = 0x11
cdef int DEFAULT_CONSONANT_MASK = 0x12
cdef int DEFAULT_DIGIT_MASK = 0x13
cdef int DEFAULT_NWS_MASK = 0x10
cdef int DEFAULT_GENERAL_MASK = 0x14
cdef int DEFAULT_2BYTE_MASK = 0x0378
cdef int DEFAULT_3BYTE_MASK = 0xF8FF
cdef int DEFAULT_4BYTE_MASK = 0x1D37F
cdef class DefaultMask:
    VOWEL: int = DEFAULT_VOWEL_MASK
    CONSONANT: int = DEFAULT_CONSONANT_MASK
    DIGIT: int = DEFAULT_DIGIT_MASK
    NWS: int = DEFAULT_NWS_MASK
    GENERAL: int = DEFAULT_GENERAL_MASK
    TWO_BYTE: int = DEFAULT_2BYTE_MASK
    THREE_BYTE: int = DEFAULT_3BYTE_MASK
    FOUR_BYTE: int = DEFAULT_4BYTE_MASK

# ---------------------------------------------------------------------
# Inline helpers
cdef inline bint is_vowel(int c):
    c = Py_UNICODE_TOLOWER(c)
    return c in (ord('a'), ord('e'), ord('i'), ord('o'), ord('u'))

cdef inline bint is_consonant(int c):
    cdef int lower_c = Py_UNICODE_TOLOWER(c)
    return Py_UNICODE_ISALPHA(lower_c) and not is_vowel(lower_c)

cdef inline bint is_digit(int c):
    return Py_UNICODE_ISDECIMAL(c) != 0

cdef inline int get_codepoint_utf8_length(int ch):
    if ch <= 0x7F:
        return 1
    elif ch <= 0x7FF:
        return 2
    elif ch <= 0xFFFF:
        return 3
    else:
        return 4


# ---------------------------------------------------------------------
# Main masking function
def random_masking(
    text: str,
    probability: float = 0.1,
    min_consecutive: int = 1,
    max_consecutive: int = 2,
    vowel_mask: int = DEFAULT_VOWEL_MASK,
    consonant_mask: int = DEFAULT_CONSONANT_MASK,
    digit_mask: int = DEFAULT_DIGIT_MASK,
    nws_mask: int = DEFAULT_NWS_MASK,
    general_mask: int = DEFAULT_GENERAL_MASK,
    two_byte_mask: int = DEFAULT_2BYTE_MASK,
    three_byte_mask: int = DEFAULT_3BYTE_MASK,
    four_byte_mask: int = DEFAULT_4BYTE_MASK,
    general_mask_probability: float = 0.5,
    seed: int = -1,
    skip_digits: int = 0,
    debug: int = 0
) -> str:
    """
    Apply random masking to `text` with the given parameters.
    Returns a new Python string with masked characters.
    """
    cdef int n = len(text)
    if n == 0:
        return ""

    # Validate some arguments
    if not 0.0 <= probability <= 1.0:
        raise ValueError("probability must be between 0 and 1.")
    if min_consecutive < 0 or max_consecutive < min_consecutive:
        raise ValueError("Invalid min/max consecutive values.")
    if not 0.0 <= general_mask_probability <= 1.0:
        raise ValueError("general_mask_probability must be between 0 and 1.")

    # Seed the RNG
    if seed == -1:
        random.seed()
    else:
        random.seed(seed)

    # (1) Create a typed memoryview for code points
    cdef cvarray[int] codepoints_array = cvarray(
        shape=(n,),
        itemsize=sizeof(int),
        format="i",
        mode='c'
    )
    cdef int[::1] codepoints = codepoints_array

    # Populate codepoints
    cdef int i
    for i in range(n):
        codepoints[i] = ord(text[i])

    # (2) Random masking logic
    cdef int ch, remaining, chars_to_mask, j, ch_bytes, orig_ch, mask_char

    i = 0
    while i < n:
        ch = codepoints[i]
        remaining = n - i

        if remaining < min_consecutive or Py_UNICODE_ISSPACE(ch):
            if debug:
                print(f"Debug: [index={i}] Not masking U+{ch:04X}")
            i += 1
            continue

        if random.random() < probability:
            chars_to_mask = min_consecutive + random.randint(0, max_consecutive - min_consecutive)
            if chars_to_mask > remaining:
                chars_to_mask = remaining

            for j in range(chars_to_mask):
                orig_ch = codepoints[i + j]
                ch_bytes = get_codepoint_utf8_length(orig_ch)

                mask_char = nws_mask
                if ch_bytes == 2:
                    mask_char = two_byte_mask
                elif ch_bytes == 3:
                    mask_char = three_byte_mask
                elif ch_bytes == 4:
                    mask_char = four_byte_mask
                else:
                    # ch_bytes == 1 => possibly vowel/consonant/digit
                    if is_digit(orig_ch) and skip_digits:
                        mask_char = orig_ch
                    elif is_digit(orig_ch) or is_vowel(orig_ch) or is_consonant(orig_ch):
                        if random.random() < general_mask_probability:
                            mask_char = general_mask
                        else:
                            if is_vowel(orig_ch):
                                mask_char = vowel_mask
                            elif is_consonant(orig_ch):
                                mask_char = consonant_mask
                            elif is_digit(orig_ch):
                                mask_char = digit_mask

                codepoints[i + j] = mask_char

                if debug:
                    print(f"Debug: [index={i+j}] Masking orig=U+{orig_ch:04X} => mask=U+{mask_char:04X} (ch_bytes={ch_bytes})")

            i += chars_to_mask
        else:
            if debug:
                print(f"Debug: [index={i}] Not masking U+{ch:04X} (random>prob)")
            i += 1

    # (3) Build the final output string
    cdef list result_chars = []
    # remove 'reserve(n)' because Python's list doesn't have it
    for i in range(n):
        result_chars.append(chr(codepoints[i]))

    cdef str output_string = "".join(result_chars)

    if debug:
        print(f"Debug: Final masked string = '{output_string}'")
        print(f"Debug: len(output_string) = {len(output_string)}")

    return output_string


def random_mask_batch(
    list text,
    double probability = 0.1,
    int min_consecutive = 1,
    int max_consecutive = 2,
    int vowel_mask = DEFAULT_VOWEL_MASK,
    int consonant_mask = DEFAULT_CONSONANT_MASK,
    int digit_mask = DEFAULT_DIGIT_MASK,
    int nws_mask = DEFAULT_NWS_MASK,
    int general_mask = DEFAULT_GENERAL_MASK,
    int two_byte_mask = DEFAULT_2BYTE_MASK,
    int three_byte_mask = DEFAULT_3BYTE_MASK,
    int four_byte_mask = DEFAULT_4BYTE_MASK,
    double general_mask_probability = 0.5,
    long seed = -1,
    int skip_digits = 0,
    int debug = 0
) -> list:
    """
    Applies the same 'random_masking' logic to a list of input strings.
    Returns a list of masked strings.

    Parameters:
      - text: list[str]
      - probability, min_consecutive, max_consecutive, ...
        (same as random_masking)
      - seed: if != -1, used to seed the RNG once; else random.seed() with system time
      - debug: if nonzero, print debug info

    Example:
      masked_list = random_mask_batch(["Hello", "World"], probability=0.2, debug=1)
    """
    cdef Py_ssize_t n = len(text)
    cdef list results = [None] * n

    cdef Py_ssize_t i
    cdef str s, masked

    if seed == -1:
        random.seed()
    else:
        random.seed(seed)

    if probability < 0.0 or probability > 1.0:
        raise ValueError("probability must be between 0 and 1.")
    if min_consecutive < 0 or max_consecutive < min_consecutive:
        raise ValueError("Invalid min/max consecutive values.")

    for s in text:
        if not isinstance(s, str):
            raise TypeError("All elements in text must be str")

    for i in range(n):
        s = text[i]
        masked = random_masking(
            s,
            probability=probability,
            min_consecutive=min_consecutive,
            max_consecutive=max_consecutive,
            vowel_mask=vowel_mask,
            consonant_mask=consonant_mask,
            digit_mask=digit_mask,
            nws_mask=nws_mask,
            general_mask=general_mask,
            two_byte_mask=two_byte_mask,
            three_byte_mask=three_byte_mask,
            four_byte_mask=four_byte_mask,
            general_mask_probability=general_mask_probability,
            seed=-1,
            skip_digits=skip_digits,
            debug=debug
        )
        results[i] = masked

    return results


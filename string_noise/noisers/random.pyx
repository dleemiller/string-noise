from libc.stdlib cimport rand, srand
from libc.time cimport clock
import random

def validate_args(input_string: str, charset: str, 
                  min_chars_in: int, max_chars_in: int, 
                  min_chars_out: int, max_chars_out: int, 
                  probability: float, seed: int, debug: int) -> bool:
    if not input_string or not isinstance(input_string, str):
        raise TypeError("input_string must be a non-empty string.")
    if not charset or not isinstance(charset, str):
        raise TypeError("charset must be a non-empty string.")
    if min_chars_in < 0 or max_chars_in < min_chars_in:
        raise ValueError("Invalid min/max chars_in values.")
    if min_chars_out < 0 or max_chars_out < min_chars_out:
        raise ValueError("Invalid min/max chars_out values.")
    if probability < 0.0 or probability > 1.0:
        raise ValueError("Probability must be between 0 and 1.")
    return True


cpdef str random_replacement(
    str input_string,
    str charset,
    int min_chars_in=1,
    int max_chars_in=2,
    int min_chars_out=1,
    int max_chars_out=2,
    double probability=0.1,
    long seed=-1,
    int debug=0
):
    if seed == -1:
        srand(<unsigned int>clock())
    else:
        srand(<unsigned int>seed)

    validate_args(input_string, charset, min_chars_in, max_chars_in, min_chars_out, max_chars_out, probability, seed, debug)

    # Initialize the output list for efficient appending
    cdef list output = []
    cdef int input_len = len(input_string)
    cdef int charset_len = len(charset)

    if debug:
        print("Debug info:")
        print(f"Original string: {input_string}")
        print(f"Charset: {charset}")

    cdef int i = 0
    while i < input_len:
        char = input_string[i]
        remaining_length = input_len - i

        if debug:
            print(f"Processing character '{char}' at index {i}")

        if remaining_length < min_chars_in or char.isspace():
            output.append(char)
            i += 1
            continue

        if random.random() < probability:
            chars_in = min_chars_in + (rand() % (max_chars_in - min_chars_in + 1))
            chars_out = min_chars_out + (rand() % (max_chars_out - min_chars_out + 1))
            chars_in = min(chars_in, remaining_length)

            for _ in range(chars_out):
                output.append(charset[rand() % charset_len])

            i += chars_in
        else:
            output.append(char)
            i += 1

    if debug:
        print("Debug: Final output length:", len(output))

    return "".join(output)


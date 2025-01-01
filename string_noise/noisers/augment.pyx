# augment_string.pyx
# cython: language_level=3
# cython: boundscheck=False, wraparound=False

"""
A Cython implementation of 'augment_string' that applies a user-defined mapping
to randomly replace substrings in an input string.

Example usage:

    from string_noise.noisers.augment_string import augment_string

    output_str = augment_string(
        input_string="Hello world",
        replacement_mapping={"world": "planet"},
        probability=0.5,
        debug=True,
        sort_order=3,  # or 0,1,2
        seed=12345
    )
"""

#########################################################
# Imports
#########################################################

import random
import math
from cython.view cimport array as cvarray
from cython cimport sizeof

# You can cimport various low-level C APIs as needed, e.g.:
# from libc.stdlib cimport srand, rand
# from libc.time cimport clock

# For maximum similarity, define same constants as your "constants.h"
cdef int ASCENDING = 0
cdef int DESCENDING = 1
cdef int SHUFFLE = 2
cdef int RESHUFFLE = 3


#########################################################
# Internal Helpers
#########################################################

cdef inline int compare_pyobject_ascending(object a, object b):
    """
    Return <0 if a < b, 0 if a == b, >0 if a > b
    We'll return 1 if a > b, -1 if a < b, 0 if equal
    to emulate "ascending" sort in a compare function.
    """
    if a < b:
        return -1
    elif a > b:
        return 1
    else:
        return 0

cdef inline int compare_pyobject_descending(object a, object b):
    """
    Return <0 if a > b, 0 if a == b, >0 if a < b
    to emulate "descending" sort logic.
    """
    if a > b:
        return -1
    elif a < b:
        return 1
    else:
        return 0


cdef void shuffle_list(list arr):
    """
    Fisher-Yates shuffle (in-place) for a Python list.
    Equivalent to the C shuffle_pyobject_array in your code.
    """
    cdef Py_ssize_t i, j, n = len(arr)
    for i in range(n - 1, 0, -1):
        j = random.randint(0, i)  # inclusive
        arr[i], arr[j] = arr[j], arr[i]


cdef int weighted_choice(double[:] weights):
    """
    Given a 1D memoryview of weights (which sum to ~1.0),
    return an index [0..len(weights)-1], chosen by random draw.
    If the sum of weights < 1.0, we'll just compare cumulatively.
    """
    cdef double total_weight = 0.0
    cdef double r = random.random()  # in [0,1)
    cdef Py_ssize_t i, count = weights.shape[0]

    # Accumulate and compare
    for i in range(count):
        total_weight += weights[i]
        if r <= total_weight:
            return i

    return -1  # fallback if floating mismatch


cdef object select_replacement(object value, bint debug):
    """
    If value is a dict[str->float], pick a weighted random key.
    If value is a list[str], pick uniformly.
    Else, value is a str => return it directly.
    """
    # -- All cdef declarations must be here, before logic begins --
    cdef object replacement = None
    cdef Py_ssize_t replacement_count = 0
    cdef list keys = []
    cdef list vals = []
    cdef cvarray[double] wts_arr  # We'll create it in a block below
    cdef double[::1] wts          # Memoryview
    cdef int idx = -1
    cdef Py_ssize_t i
    cdef double sum_w = 0.0

    # -------------------- Logic starts here --------------------
    if isinstance(value, dict):
        # Weighted dictionary
        replacement_count = len(value)
        if replacement_count == 0:
            return None

        keys = list(value.keys())
        vals = list(value.values())

        # Allocate a 1D array of doubles
        wts_arr = cvarray(
            shape=(replacement_count,),
            itemsize=sizeof(double),
            format="d",
            mode='c'
        )
        wts = wts_arr

        sum_w = 0.0
        for i in range(replacement_count):
            wts[i] = float(vals[i])
            sum_w += wts[i]

        idx = weighted_choice(wts)
        if idx < 0 or idx >= replacement_count:
            return None

        replacement = keys[idx]

    elif isinstance(value, list):
        replacement_count = len(value)
        if replacement_count == 0:
            return None

        idx = random.randint(0, replacement_count - 1)
        replacement = value[idx]

        if debug:
            print(f"List choice replacement: {replacement}")

    else:
        # Direct string replacement
        replacement = value

    return replacement


cdef object normalize_string(object input_string):
    """
    In Python/Cython, we typically don't need a separate "normalize" pass
    from UTF-8 -> PyUnicode -> UTF-8 -> PyUnicode.
    But if you really want to replicate that logic, you can re-encode and decode.
    """
    if not isinstance(input_string, str):
        return None

    # "Normalization" approach:
    try:
        utf8_bytes = input_string.encode('utf-8', 'strict')
        normalized_str = utf8_bytes.decode('utf-8', 'strict')
        return normalized_str
    except UnicodeError:
        return None


#########################################################
# The Core Replacement Logic
#########################################################

cdef str perform_replacements(
    str input_string,
    dict replacement_mapping,
    double probability,
    bint debug,
    int sort_order
):
    """
    Translate the C function 'perform_replacements' into Cython/Python logic.
    """

    cdef Py_ssize_t input_len = len(input_string)
    cdef list keys = list(replacement_mapping.keys())
    cdef Py_ssize_t key_count = len(keys)

    # Sort or shuffle keys array
    if sort_order == ASCENDING:
        if debug:
            print("SORT ASCENDING...")
        keys.sort(key=lambda x: x)  # ascending
    elif sort_order == DESCENDING:
        if debug:
            print("SORT DESCENDING...")
        keys.sort(key=lambda x: x, reverse=True)
    elif sort_order == SHUFFLE:
        if debug:
            print("SORT SHUFFLE...")
        shuffle_list(keys)
    # We handle RESHUFFLE later inside the loop, if needed

    cdef list output_chars = []
    cdef Py_ssize_t i = 0
    cdef Py_ssize_t key_len
    cdef bint replaced = False
    cdef object key
    cdef object value
    cdef object replacement

    while i < input_len:
        # Probability check
        if random.random() > probability:
            # Just copy this char
            output_chars.append(input_string[i])
            i += 1
            continue

        # Possibly re-shuffle if sort_order == RESHUFFLE
        if sort_order == 3:  # RESHUFFLE
            if debug:
                print("SORT RESHUFFLE...")
            shuffle_list(keys)

        # Attempt each key in order
        for key in keys:
            key_len = len(key)  # length of this key
            if i + key_len > input_len:
                continue

            # Check if substring from i..i+key_len matches key
            if input_string[i : i + key_len] == key:
                # We got a match => pick a replacement
                value = replacement_mapping[key]
                replacement = select_replacement(value, debug)
                if replacement is None:
                    # Something went wrong or user gave empty dict/list
                    # fallback => just copy the char
                    output_chars.append(input_string[i])
                    i += 1
                    replaced = True
                    break

                # replacement must be str
                if not isinstance(replacement, str):
                    # fallback => copy the single char
                    output_chars.append(input_string[i])
                    i += 1
                    replaced = True
                    break

                # Append the entire replacement
                output_chars.append(replacement)
                i += key_len
                replaced = True
                break

        if not replaced:
            # no key matched => copy char
            output_chars.append(input_string[i])
            i += 1

    cdef str joined_output = "".join(output_chars)

    # "Normalize" if you want to replicate the original logic:
    cdef object normalized = normalize_string(joined_output)
    if normalized is None:
        return joined_output  # fallback if error
    return <str>normalized


#########################################################
# Public API Function
#########################################################

def augment_string(
    input_string: str,
    replacement_mapping: dict,
    probability: float = 1.0,
    debug: bool = False,
    sort_order: int = 3,  # default = RESHUFFLE
    seed: int = -1
) -> str:
    """
    Applies the user-supplied 'replacement_mapping' to randomly corrupt 'input_string'.
    - `replacement_mapping` can have:
      - str -> str
      - str -> list[str]
      - str -> dict[str,float] (weighted replacements)
    - `probability`: chance to attempt a replacement at each character.
    - `debug`: if True, print debug info.
    - `sort_order` in {0=ASCENDING, 1=DESCENDING, 2=SHUFFLE, 3=RESHUFFLE}.

    Returns the augmented string.
    Raises ValueError or TypeError on invalid arguments.
    """

    # Basic argument checks
    if not isinstance(input_string, str):
        raise TypeError("input_string must be a string")
    if not isinstance(replacement_mapping, dict):
        raise TypeError("replacement_mapping must be a dict")
    if probability < 0.0 or probability > 1.0:
        raise ValueError("probability must be between 0 and 1")

    if seed == -1:
        # seed with system time or any random seed
        random.seed()
    else:
        random.seed(seed)

    # Validate the replacement mapping
    if not _validate_replacement_mapping(replacement_mapping):
        raise ValueError("Invalid replacement_mapping structure")

    # Actually do replacements
    cdef str out = perform_replacements(
        input_string=input_string,
        replacement_mapping=replacement_mapping,
        probability=probability,
        debug=debug,
        sort_order=sort_order
    )
    return out


def augment_batch(
    list input_strings, 
    dict replacement_mapping, 
    double probability=1.0,
    bint debug=False,
    int sort_order=3,   # e.g. 3=RESHUFFLE in your code
    int seed=-1
) -> list:
    """
    Apply the same 'augment_string' logic to a list of strings.
    
    - input_strings: list[str]
    - replacement_mapping: same dict as used in augment_string
    - probability: chance to apply a replacement at each character
    - debug: whether to print debug info
    - sort_order: 0=ASC, 1=DESC, 2=SHUFFLE, 3=RESHUFFLE
    - seed: random seed (if -1, use system time)

    Returns a list[str] of augmented strings, one per input string.
    """
    cdef str s

    # Basic argument checks
    if seed == -1:
        random.seed()
    else:
        random.seed(seed)

    if not (0.0 <= probability <= 1.0):
        raise ValueError("probability must be between 0 and 1.")

    if not isinstance(replacement_mapping, dict):
        raise TypeError("replacement_mapping must be a dict.")

    for s in input_strings:
        if not isinstance(s, str):
            raise TypeError("All elements in input_strings must be str")

    if not _validate_replacement_mapping(replacement_mapping):
        raise ValueError("Invalid replacement_mapping structure")

    cdef Py_ssize_t n = len(input_strings)
    cdef list results = [None] * n

    cdef Py_ssize_t i
    cdef str out

    for i in range(n):
        s = input_strings[i]
        out = perform_replacements(
            input_string=s,
            replacement_mapping=replacement_mapping,
            probability=probability,
            debug=debug,
            sort_order=sort_order
        )
        results[i] = out

    return results


#########################################################
# Additional Validation Routines
#########################################################

cdef bint _validate_replacement_mapping(dict mapping):
    """
    Recreate the checks you had in validate_replacement_mapping:
    - keys must be str
    - values must be str, list[str], or dict[str->float]
    - for lists => must not be empty, elements must be str
    - for dict => keys must be str, values float
    """
    cdef object k, v
    for k, v in mapping.items():
        if not isinstance(k, str):
            return False

        # If v is a dict => check subkeys are str, subvals are float
        if isinstance(v, dict):
            if not v:
                return False
            for subk, subval in v.items():
                if not isinstance(subk, str):
                    return False
                if not (isinstance(subval, float) or isinstance(subval, int)):
                    return False
        elif isinstance(v, list):
            if len(v) == 0:
                return False
            for item in v:
                if not isinstance(item, str):
                    return False
        elif not isinstance(v, str):
            return False

    return True


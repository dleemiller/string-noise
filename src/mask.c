#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "constants.h"
#include "utils.h"

int is_vowel(Py_UCS4 c) {
    Py_UCS4 lower_c = Py_UNICODE_TOLOWER(c);
    return (lower_c == 'a' || lower_c == 'e' || lower_c == 'i' || lower_c == 'o' || lower_c == 'u');
}

int is_consonant(Py_UCS4 c) {
    Py_UCS4 lower_c = Py_UNICODE_TOLOWER(c);
    return Py_UNICODE_ISALPHA(lower_c) && !is_vowel(lower_c);
}

int is_digit(Py_UCS4 c) {
    return Py_UNICODE_ISDECIMAL(c);
}

int validate_masking_args(PyObject *input_string, double probability, 
                          int min_consecutive, int max_consecutive, 
                          Py_UCS4 vowel_mask, Py_UCS4 consonant_mask, 
                          Py_UCS4 digit_mask, Py_UCS4 nws_mask, 
                          Py_UCS4 general_mask, Py_UCS4 two_byte_mask, 
                          Py_UCS4 four_byte_mask, double general_mask_probability, 
                          long seed, int skip_digits, int debug) {

    // Validate input_string
    if (!PyUnicode_Check(input_string)) {
        PyErr_SetString(PyExc_TypeError, "input_string must be a string.");
        return 0;
    }

    // Validate probability
    if (probability < 0.0 || probability > 1.0) {
        PyErr_SetString(PyExc_ValueError, "Probability must be between 0 and 1.");
        return 0;
    }

    // Validate min and max consecutive characters
    if (min_consecutive < 0 || max_consecutive < min_consecutive) {
        PyErr_SetString(PyExc_ValueError, "Invalid min/max consecutive values.");
        return 0;
    }

    // Validate general_mask_probability
    if (general_mask_probability < 0.0 || general_mask_probability > 1.0) {
        PyErr_SetString(PyExc_ValueError, "General mask probability must be between 0 and 1.");
        return 0;
    }

    return 1;
}


PyObject* random_masking(PyObject *self, PyObject *args, PyObject *keywds) {
    PyObject *input_string;
    double probability = 0.1;
    int min_consecutive = 1;
    int max_consecutive = 2;
    Py_UCS4 vowel_mask = DEFAULT_VOWEL_MASK;
    Py_UCS4 consonant_mask = DEFAULT_CONSONANT_MASK;
    Py_UCS4 digit_mask = DEFAULT_DIGIT_MASK;
    Py_UCS4 nws_mask = DEFAULT_NWS_MASK;
    Py_UCS4 general_mask = DEFAULT_GENERAL_MASK;
    Py_UCS4 two_byte_mask = DEFAULT_2BYTE_MASK;
    Py_UCS4 four_byte_mask = DEFAULT_4BYTE_MASK;
    double general_mask_probability = 0.5;
    long seed = -1;
    int skip_digits = 0;
    int debug = 0;
    static char *kwlist[] = {"input_string", "probability", "min_consecutive", "max_consecutive",
                             "vowel_mask", "consonant_mask", "digit_mask", "nws_mask", "general_mask", 
                             "two_byte_mask", "four_byte_mask", "general_mask_probability", 
                             "seed", "skip_digits", "debug", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "U|diIIIIIIIIdlii", kwlist, &input_string,
                                     &probability, &min_consecutive, &max_consecutive, 
                                     &vowel_mask, &consonant_mask, &digit_mask, &nws_mask, &general_mask, 
                                     &two_byte_mask, &four_byte_mask, &general_mask_probability, &seed,
                                     &skip_digits, &debug)) {
        return NULL;
    }

    // After parsing the arguments with PyArg_ParseTupleAndKeywords
    if (!validate_masking_args(input_string, probability, min_consecutive, 
                               max_consecutive, vowel_mask, consonant_mask, 
                               digit_mask, nws_mask, general_mask, 
                               two_byte_mask, four_byte_mask, 
                               general_mask_probability, seed, skip_digits, debug)) {
        return NULL; // If validation fails, return NULL. Error is set in validate_masking_args
    }

    // Seed the random number generator
    if (seed == -1) {
        srand((unsigned int)clock());
    } else {
        srand((unsigned int)seed);
    }

    Py_ssize_t input_len = PyUnicode_GET_LENGTH(input_string);
    const Py_ssize_t buffer_margin = 64;

    // Use the utility function to get aligned size
    Py_ssize_t aligned_input_len = get_aligned_size(input_len);

    PyObject *output_string = PyUnicode_New(aligned_input_len, PyUnicode_MAX_CHAR_VALUE(input_string));
    if (!output_string) {
        PyErr_NoMemory();
        return NULL;
    }

    Py_ssize_t output_len = 0;
    for (Py_ssize_t i = 0; i < input_len;) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(input_string, i);
        int remaining_length = input_len - i;
    
        if (debug) {
            printf("Processing character %c at index %zu\n", ch, i);
        }
    
        if (remaining_length < min_consecutive || Py_UNICODE_ISSPACE(ch)) {
            if (!write_char_to_output(&output_string, &output_len, ch, buffer_margin, debug)) {
                PyErr_NoMemory();
                Py_DECREF(output_string);
                return NULL;
            }
            i++;
            continue;
        }

        if ((double)rand() / RAND_MAX < probability) {
            int chars_to_mask = min_consecutive + (rand() % (max_consecutive - min_consecutive + 1));
            chars_to_mask = process_chars_in(input_string, i, chars_to_mask);
            chars_to_mask = (chars_to_mask > remaining_length) ? remaining_length : chars_to_mask;
    
            if (chars_to_mask == 0) {
                if (!write_char_to_output(&output_string, &output_len, ch, buffer_margin, debug)) {
                    PyErr_NoMemory();
                    Py_DECREF(output_string);
                    return NULL;
                }
            }

            for (int j = 0; j < chars_to_mask; ++j) {
                ch = PyUnicode_READ_CHAR(input_string, i + j);

                Py_UCS4 mask_char = nws_mask;

                if ((ch & 0xE0) == 0xC0) mask_char = two_byte_mask;
                else if ((ch & 0xF0) == 0xE0 || (ch & 0xF8) == 0xF0) mask_char = four_byte_mask;
                else if (is_vowel(ch) || is_consonant(ch) || is_digit(ch)) {
                    mask_char = ((double)rand() / RAND_MAX < general_mask_probability) ? general_mask : 
                                (is_vowel(ch) ? vowel_mask : (is_consonant(ch) ? consonant_mask : digit_mask));

                    if (skip_digits && is_digit(ch)) {
                        mask_char = ch; // no masking
                    }
                }

                if (!write_char_to_output(&output_string, &output_len, mask_char, buffer_margin, debug)) {
                    Py_DECREF(output_string);
                    return PyErr_NoMemory();
                }
            }
            i += chars_to_mask;
        } else {
            if (!write_char_to_output(&output_string, &output_len, ch, buffer_margin, debug)) {
                Py_DECREF(output_string);
                return PyErr_NoMemory();
            }
            i++;
        }
    }

    // Resize output string to match output_len
    if (PyUnicode_Resize(&output_string, output_len) < 0) {
        Py_DECREF(output_string);
        return PyErr_NoMemory();
    }

    return output_string;
}


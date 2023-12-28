#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "utils.h"


int validate_args(PyObject *input_string, PyObject *charset, 
                  int min_chars_in, int max_chars_in, 
                  int min_chars_out, int max_chars_out, 
                  double probability, long seed, int debug) {

    // Validate input_string and charset
    if (!PyUnicode_Check(input_string)) {
        PyErr_SetString(PyExc_TypeError, "input_string must be a string.");
        return 0;
    }
    if (!PyUnicode_Check(charset)) {
        PyErr_SetString(PyExc_TypeError, "charset must be a string.");
        return 0;
    }

    // Ensure charset is not empty
    if (PyUnicode_GET_LENGTH(charset) == 0) {
        PyErr_SetString(PyExc_ValueError, "Charset cannot be empty.");
        return 0;
    }

    // Validate min and max chars in/out
    if (min_chars_in < 0 || max_chars_in < min_chars_in) {
        PyErr_SetString(PyExc_ValueError, "Invalid min/max chars_in values.");
        return 0;
    }
    if (min_chars_out < 0 || max_chars_out < min_chars_out) {
        PyErr_SetString(PyExc_ValueError, "Invalid min/max chars_out values.");
        return 0;
    }

    // Validate probability
    if (probability < 0.0 || probability > 1.0) {
        PyErr_SetString(PyExc_ValueError, "Probability must be between 0 and 1.");
        return 0;
    }

    return 1;
}

PyObject* random_replacement(PyObject *self, PyObject *args, PyObject *keywds) {
    PyObject *input_string, *charset;
    int min_chars_in = 1;
    int max_chars_in = 2;
    int min_chars_out = 1;
    int max_chars_out = 2;
    double probability = 0.1;
    long seed = -1;
    int debug = 0;  

    static char *kwlist[] = {"input_string", "charset", "min_chars_in", "max_chars_in", 
                             "min_chars_out", "max_chars_out", "probability", "seed", "debug", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "UU|iiiidli", kwlist, 
                                     &input_string, &charset, &min_chars_in, &max_chars_in, 
                                     &min_chars_out, &max_chars_out, &probability, &seed, &debug)) {
        return NULL;
    }

    if (seed == -1) {
        srand((unsigned int)clock());
    } else {
        srand((unsigned int)seed);
    }

    Py_ssize_t input_len = PyUnicode_GET_LENGTH(input_string);
    Py_ssize_t charset_len = PyUnicode_GET_LENGTH(charset);
    if (!validate_args(input_string, charset, min_chars_in, max_chars_in, 
                       min_chars_out, max_chars_out, probability, seed, debug)) {
        return NULL; // Validation failed
    }

    const Py_UCS4 max_input_char = PyUnicode_MAX_CHAR_VALUE(input_string);
    const Py_UCS4 max_charset_char = PyUnicode_MAX_CHAR_VALUE(charset);
    const Py_UCS4 max_char_value = max_input_char > max_charset_char ? max_input_char : max_charset_char;

    const Py_ssize_t buffer_margin = 64; // Set a margin for buffer resizing
    PyObject *output_string = PyUnicode_New(input_len + buffer_margin, max_char_value);
    if (!output_string) {
        PyErr_NoMemory();
        return NULL;
    }

    if (debug) {
        printf("Debug info: \n");
        printf("Original string: %s\n", PyUnicode_AsUTF8(input_string));
        printf("Charset: %s\n", PyUnicode_AsUTF8(charset));
    }

    Py_ssize_t output_len = 0;
    for (Py_ssize_t i = 0; i < input_len;) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(input_string, i);
        int remaining_length = input_len - i;
    
        if (debug) {
            printf("Processing character %c at index %zu\n", ch, i);
        }
    
        if (remaining_length < min_chars_in || Py_UNICODE_ISSPACE(ch)) {
            if (!write_char_to_output(&output_string, &output_len, ch, buffer_margin, debug)) {
                PyErr_NoMemory();
                Py_DECREF(output_string);
                return NULL;
            }
            i++;
            continue;
        }
    
        if ((double)rand() / RAND_MAX < probability) {
            int chars_in = min_chars_in + (rand() % (max_chars_in - min_chars_in + 1));
            int chars_out = min_chars_out + (rand() % (max_chars_out - min_chars_out + 1));
    
            chars_in = process_chars_in(input_string, i, chars_in);
            chars_in = (chars_in > remaining_length) ? remaining_length : chars_in;
    
            if (chars_in == 0) {
                if (!write_char_to_output(&output_string, &output_len, ch, buffer_margin, debug)) {
                    PyErr_NoMemory();
                    Py_DECREF(output_string);
                    return NULL;
                }
            }
    
            for (int j = 0; j < chars_out; ++j) {
                if (output_len >= PyUnicode_GET_LENGTH(output_string)) {
                    if (!write_char_to_output(&output_string, &output_len, ch, buffer_margin, debug)) {
                        PyErr_NoMemory();
                        Py_DECREF(output_string);
                        return NULL;
                    }
                }
                Py_UCS4 new_char = PyUnicode_READ_CHAR(charset, rand() % charset_len);
                if (!write_char_to_output(&output_string, &output_len, new_char, buffer_margin, debug)) {
                    PyErr_NoMemory();
                    Py_DECREF(output_string);
                    return NULL;
                }
            }
    
            i += (chars_in > 0 ? chars_in : 1);
        } else {
            if (!write_char_to_output(&output_string, &output_len, ch, buffer_margin, debug)) {
                PyErr_NoMemory();
                Py_DECREF(output_string);
                return NULL;
            }
            i++;
        }
    }

    if (debug) {
        printf("Debug: Final output_len: %zd before resizing\n", output_len);
    }

    // Resize output string to match output_len
    if (PyUnicode_Resize(&output_string, output_len) < 0) {
        Py_DECREF(output_string);
        return PyErr_NoMemory();
    }

    return output_string;
}


#include "utils.h"
#include <Python.h>
#include <stdio.h>


Py_ssize_t get_aligned_size(Py_ssize_t size) {
    const Py_ssize_t alignment = 64;
    return (size + alignment - 1) / alignment * alignment;
}


// Function to determine the number of characters to be replaced, adjusting for whitespace
Py_ssize_t process_chars_in(PyObject *input_string, Py_ssize_t start, Py_ssize_t chars_in) {
    Py_ssize_t input_len = PyUnicode_GET_LENGTH(input_string);
    for (Py_ssize_t j = 0; j < chars_in && start + j < input_len; ++j) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(input_string, start + j);
        if (Py_UNICODE_ISSPACE(ch)) {
            return j;  // Return the new length up to the whitespace character
        }
    }
    return chars_in;  // Return the original length if no whitespace is encountered
}


int ensure_buffer_capacity(PyObject **output_string, Py_ssize_t required_capacity, Py_ssize_t margin, int debug) {
    Py_ssize_t current_capacity = PyUnicode_GET_LENGTH(*output_string);
    if (debug) {
        printf("Debug: ensure_buffer_capacity - Current Capacity: %zd, Required Capacity: %zd\n", current_capacity, required_capacity);
    }

    if (required_capacity + margin > current_capacity) {
        Py_ssize_t new_capacity = current_capacity + margin;
        if (debug) {
            printf("Debug: Resizing output buffer to new capacity: %zd\n", new_capacity);
        }
        if (PyUnicode_Resize(output_string, new_capacity) < 0) {
            if (debug) {
                printf("Debug: Buffer resizing failed\n");
            }
            return 0; // Indicates failure in resizing
        }
    }
    return 1; // Indicates success
}


int write_char_to_output(PyObject **output_string, Py_ssize_t *output_len, Py_UCS4 ch, Py_ssize_t margin, int debug) {
    if (!ensure_buffer_capacity(output_string, *output_len + 1, margin, debug)) {
        return 0;  // Failure in buffer capacity check or resizing
    }
    PyUnicode_WRITE(PyUnicode_KIND(*output_string), PyUnicode_DATA(*output_string), (*output_len)++, ch);
    if (debug) {
        printf("Debug: Char written to output: %c (0x%x), New output_len: %zd\n", ch, ch, *output_len);
    }
    return 1;  // Success
}


// Prints a Unicode object for debugging purposes
void debug_print_unicode(PyObject *unicode_obj, const char *label) {
    if (unicode_obj != NULL && label != NULL) {
        PyObject *repr = PyObject_Str(unicode_obj);
        if (repr != NULL) {
            const char *cstr = PyUnicode_AsUTF8(repr);
            if (cstr != NULL) {
                printf("%s: %s\n", label, cstr);
            } else {
                PyErr_Print();  // Handle potential error in conversion
            }
            Py_DECREF(repr);  // Clean up the temporary Python string object
        } else {
            PyErr_Print();  // Handle potential error in conversion
        }
    }
}


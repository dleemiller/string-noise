#ifndef STRING_NOISE_UTILS_H
#define STRING_NOISE_UTILS_H

#include <Python.h>

Py_ssize_t get_aligned_size(Py_ssize_t size);
Py_ssize_t process_chars_in(PyObject *input_string, Py_ssize_t start, Py_ssize_t chars_in);
int ensure_buffer_capacity(PyObject **output_string, Py_ssize_t required_capacity, Py_ssize_t margin, int debug);
int write_char_to_output(PyObject **output_string, Py_ssize_t *output_len, Py_UCS4 ch, Py_ssize_t margin, int debug);
void debug_print_unicode(PyObject *unicode_obj, const char *label);

#endif // UTILS_H


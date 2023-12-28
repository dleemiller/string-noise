#ifndef RANDOM_REPLACEMENT_H
#define RANDOM_REPLACEMENT_H

#include <Python.h>

int process_chars_in(char *original_string, size_t start, int chars_in);

// Function declaration
PyObject* random_replacement(PyObject *self, PyObject *args);

#endif


#ifndef MASK_H
#define MASK_H

#include <Python.h>

// Function Prototypes
int is_vowel(char c);
int is_consonant(char c);
int process_chars_in(char *original_string, size_t start, int max_chars);
PyObject* random_masking(PyObject *self, PyObject *args, PyObject *keywds);

#endif // MASK_H


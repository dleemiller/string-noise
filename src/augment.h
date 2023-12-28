#ifndef STRING_NOISE_AUGMENT_H
#define STRING_NOISE_AUGMENT_H

#include <Python.h>

extern PyObject* AugmentationFailed;

// Declaration of the augment_string function
PyObject* augment_string(PyObject* self, PyObject* args, PyObject* kwds);

#endif // STRING_NOISE_AUGMENT_H


#ifndef AUGMENT_STRING_H
#define AUGMENT_STRING_H

#include <Python.h>

extern PyObject* AugmentationFailed;

// Declaration of the augment_string function
PyObject* augment_string(PyObject* self, PyObject* args, PyObject* kwds);

#endif // AUGMENT_STRING_H


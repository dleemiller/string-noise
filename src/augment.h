#ifndef AUGMENT_H
#define AUGMENT_H

#include <Python.h>

extern PyObject* AugmentationFailed;

// Declaration of the augment_string function
PyObject* augment_string(PyObject* self, PyObject* args, PyObject* kwds);

#endif // AUGMENT_H


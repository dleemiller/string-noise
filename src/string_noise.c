#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

PyObject *AugmentationFailed;

#include "normalize.h"
#include "augment.h"



// Method definitions
static PyMethodDef StringNoiseMethods[] = {
    {
        "augment_string", (PyCFunction)augment_string, METH_VARARGS | METH_KEYWORDS,
        "Augments a string based on the specified mapping and probability."
    },
    {"normalize", (PyCFunction)normalize, METH_VARARGS | METH_KEYWORDS, "Normalize the values in a dictionary or list"},
    {NULL, NULL, 0, NULL} // Sentinel
};

// Module definition
static struct PyModuleDef stringnoisemodule = {
    PyModuleDef_HEAD_INIT,
    "string_noise",
    "Module for augmenting strings with noise.",
    -1,
    StringNoiseMethods
};

// Module initialization
PyMODINIT_FUNC PyInit_string_noise(void) {
    PyObject *module = PyModule_Create(&stringnoisemodule);
    if (module == NULL) {
        return NULL;
    }

    // Define constants for sorting order
    PyModule_AddIntConstant(module, "SHUFFLE", 0);
    PyModule_AddIntConstant(module, "ASCENDING", 1);
    PyModule_AddIntConstant(module, "DESCENDING", 2);
    PyModule_AddIntConstant(module, "RESHUFFLE", 3);

    srand((unsigned int)time(NULL));  // Seed the random number generator

    // Create the AugmentationFailed exception
    AugmentationFailed = PyErr_NewException("string_noise.AugmentationFailed", NULL, NULL);
    Py_XINCREF(AugmentationFailed);
    if (PyModule_AddObject(module, "AugmentationFailed", AugmentationFailed) < 0) {
        Py_XDECREF(AugmentationFailed);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}



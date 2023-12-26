#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
    if (module != NULL) {
        srand((unsigned int)time(NULL));  // Seed the random number generator
    }
    return module;
}

#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

PyObject *AugmentationFailed;

#include "constants.h"
#include "utils.h"
#include "normalize.h"
#include "augment.h"
#include "random.h"
#include "mask.h"
#include "trie.h"
#include "markov.h"


// Method definitions
static PyMethodDef StringNoiseMethods[] = {
    {
        "augment_string", (PyCFunction)augment_string, METH_VARARGS | METH_KEYWORDS,
        "Augments a string based on the specified mapping and probability."
    },
    {"normalize", (PyCFunction)normalize, METH_VARARGS | METH_KEYWORDS, "Normalize the values in a dictionary or list"},
    {"random_replacement", (PyCFunction)random_replacement, METH_VARARGS | METH_KEYWORDS,
     "Randomly replace characters in a string."},
    {"random_masking", (PyCFunction)random_masking, METH_VARARGS | METH_KEYWORDS, "Randomly masks a string."},
    //{"tokenize", (PyCFunction)tokenize, METH_VARARGS | METH_KEYWORDS, "Tokenize a string."},
    {
        "build_tree", (PyCFunction)build_tree, METH_VARARGS,
        "Build the Trie from a dictionary of mappings."
    },
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
    PyModule_AddIntConstant(module, "DEFAULT_VOWEL_MASK", DEFAULT_VOWEL_MASK);
    PyModule_AddIntConstant(module, "DEFAULT_CONSONANT_MASK", DEFAULT_CONSONANT_MASK);
    PyModule_AddIntConstant(module, "DEFAULT_DIGIT_MASK", DEFAULT_DIGIT_MASK);
    PyModule_AddIntConstant(module, "DEFAULT_GENERAL_MASK", DEFAULT_GENERAL_MASK);
    PyModule_AddIntConstant(module, "DEFAULT_NWS_MASK", DEFAULT_NWS_MASK);
    PyModule_AddIntConstant(module, "DEFAULT_2BYTE_MASK", DEFAULT_2BYTE_MASK);
    PyModule_AddIntConstant(module, "DEFAULT_4BYTE_MASK", DEFAULT_4BYTE_MASK);

    if (PyType_Ready(&PyTrieType) < 0)
        return NULL;

    // Add PyTrieType to the module
    Py_INCREF(&PyTrieType);
    if (PyModule_AddObject(module, "Trie", (PyObject *)&PyTrieType) < 0) {
        Py_DECREF(&PyTrieType);
        Py_DECREF(module);
        return NULL;
    }

    if (PyType_Ready(&PyMarkovTrieType) < 0)
        return NULL;

    Py_INCREF(&PyMarkovTrieType);
    if (PyModule_AddObject(module, "MarkovTrie", (PyObject *)&PyMarkovTrieType) < 0) {
        Py_DECREF(&PyMarkovTrieType);
        Py_DECREF(module);
        return NULL;
    }

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



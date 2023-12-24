#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Function Prototypes
static PyObject* parse_arguments(PyObject* args, PyObject* kwds, PyObject** input_string, PyObject** replacement_mapping, double* probability, int* debug);
static int validate_replacement_mapping(PyObject* replacement_mapping);
static void shuffle_pyobject_array(PyObject** array, Py_ssize_t n);
static PyObject* perform_replacements(PyObject* input_string, PyObject* replacement_mapping, double probability, int debug);
static PyObject* string_noise_augment_string(PyObject* self, PyObject* args, PyObject* kwds);

// Utility function to shuffle an array of PyObject pointers
static void shuffle_pyobject_array(PyObject** array, Py_ssize_t n) {
    for (Py_ssize_t i = n - 1; i > 0; i--) {
        Py_ssize_t j = rand() % (i + 1);
        PyObject* temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}


// Function to parse input arguments
static PyObject* parse_arguments(PyObject* args, PyObject* kwds, PyObject** input_string, PyObject** replacement_mapping, double* probability, int* debug) {
    PyObject* debug_obj = NULL;
    static char* kwlist[] = {"input_string", "replacement_mapping", "probability", "debug", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "UO!d|O", kwlist,
                                     input_string,
                                     &PyDict_Type, replacement_mapping,
                                     probability,
                                     &debug_obj)) {
        return NULL;
    }

    if (*probability < 0.0 || *probability > 1.0) {
        PyErr_SetString(PyExc_ValueError, "Probability must be between 0 and 1");
        return NULL;
    }

    if (debug_obj && PyObject_IsTrue(debug_obj)) {
        *debug = 1;
    }

    Py_RETURN_NONE;
}


// Function to validate the replacement mapping
static int validate_replacement_mapping(PyObject* replacement_mapping) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    // Iterate over the dictionary items
    while (PyDict_Next(replacement_mapping, &pos, &key, &value)) {
        // Check if the key is a string
        if (!PyUnicode_Check(key)) {
            PyErr_SetString(PyExc_TypeError, "All keys in the replacement mapping must be strings");
            return 0; // Return 0 (false) if a key is not a string
        }

        // Check if the value is either a string or a non-empty list of strings
        if (!(PyUnicode_Check(value) || (PyList_Check(value) && PyList_Size(value) > 0))) {
            PyErr_SetString(PyExc_TypeError, "All values in the replacement mapping must be strings or non-empty lists of strings");
            return 0; // Return 0 (false) if a value is not valid
        }

        // If the value is a list, validate each item in the list
        if (PyList_Check(value)) {
            for (Py_ssize_t i = 0; i < PyList_Size(value); i++) {
                PyObject *item = PyList_GetItem(value, i);
                // Check if each item in the list is a string
                if (!PyUnicode_Check(item)) {
                    PyErr_SetString(PyExc_TypeError, "All items in the replacement mapping lists must be strings");
                    return 0; // Return 0 (false) if an item is not a string
                }
            }
        }
    }
    return 1; // Return 1 (true) if the replacement mapping is valid
}


static PyObject* perform_replacements(PyObject* input_string, PyObject* replacement_mapping, double probability, int debug) {
    Py_ssize_t input_len = PyUnicode_GET_LENGTH(input_string);
    Py_UCS4 max_char = PyUnicode_MAX_CHAR_VALUE(input_string);
    PyObject *output_string = PyUnicode_New(input_len, max_char);
    if (!output_string) {
        PyErr_NoMemory();
        return NULL;
    }

    PyObject *keys = PyDict_Keys(replacement_mapping);
    Py_ssize_t key_count = PyList_Size(keys);
    PyObject** keys_array = (PyObject**)malloc(key_count * sizeof(PyObject*));
    if (!keys_array) {
        PyErr_NoMemory();
        Py_DECREF(keys);
        Py_DECREF(output_string);
        return NULL;
    }

    // Copy the keys to the array for shuffling
    for (Py_ssize_t i = 0; i < key_count; i++) {
        keys_array[i] = PyList_GetItem(keys, i);
        Py_INCREF(keys_array[i]);
    }

    Py_ssize_t output_len = 0;
    for (Py_ssize_t i = 0; i < input_len; i++) {
        if ((double)rand() / RAND_MAX > probability) {
            Py_UCS4 ch = PyUnicode_READ_CHAR(input_string, i);
            PyUnicode_WRITE(PyUnicode_KIND(output_string), PyUnicode_DATA(output_string), output_len++, ch);
            continue;
        }

        shuffle_pyobject_array(keys_array, key_count);

        int replaced = 0;
        for (Py_ssize_t j = 0; j < key_count; j++) {
            PyObject *key = keys_array[j];
            PyObject *value = PyDict_GetItem(replacement_mapping, key);
            Py_ssize_t key_len = PyUnicode_GET_LENGTH(key);

            if (i + key_len > input_len) continue;

            PyObject *substring = PyUnicode_Substring(input_string, i, i + key_len);
            if (!substring) {
                Py_DECREF(keys);
                Py_DECREF(output_string);
                free(keys_array);
                return NULL;
            }

            int compare_result = PyUnicode_Compare(substring, key);
            Py_DECREF(substring);

            if (compare_result == 0) {
                PyObject *replacement;
                if (PyList_Check(value)) {
                    Py_ssize_t replacement_list_size = PyList_Size(value);
                    int replacement_index = rand() % replacement_list_size;
                    replacement = PyList_GetItem(value, replacement_index);
                } else {
                    replacement = value;
                }

                Py_ssize_t repl_len = PyUnicode_GET_LENGTH(replacement);
                for (Py_ssize_t k = 0; k < repl_len; k++) {
                    Py_UCS4 ch = PyUnicode_READ_CHAR(replacement, k);
                    PyUnicode_WRITE(PyUnicode_KIND(output_string), PyUnicode_DATA(output_string), output_len++, ch);
                }

                i += key_len - 1;
                replaced = 1;
                break;
            }
        }

        if (!replaced) {
            Py_UCS4 ch = PyUnicode_READ_CHAR(input_string, i);
            PyUnicode_WRITE(PyUnicode_KIND(output_string), PyUnicode_DATA(output_string), output_len++, ch);
        }
    }

    Py_DECREF(keys);
    for (Py_ssize_t i = 0; i < key_count; i++) {
        Py_DECREF(keys_array[i]);
    }
    free(keys_array);

    // Resize the output string to the actual length
    if (PyUnicode_Resize(&output_string, output_len) < 0) {
        return NULL;
    }

    return output_string;
}


static PyObject* string_noise_augment_string(PyObject* self, PyObject* args, PyObject* kwds) {
    PyObject *input_string, *replacement_mapping;
    double probability;
    int debug = 0;  // Default debug mode is off

    // Parse arguments
    if (parse_arguments(args, kwds, &input_string, &replacement_mapping, &probability, &debug) == NULL) {
        return NULL;  // Error parsing arguments
    }

    // Validate replacement mapping
    if (!validate_replacement_mapping(replacement_mapping)) {
        return NULL;  // Error in validation
    }

    //// Sort keys by length
    //PyObject **sorted_keys = sort_keys_by_length(replacement_mapping, &key_count);
    //if (!sorted_keys) {
    //    return NULL;  // Error in sorting keys
    //}

    // Perform replacements
    PyObject *output_string = perform_replacements(input_string, replacement_mapping, probability, debug);

    return output_string;
}


static PyMethodDef StringNoiseMethods[] = {
    {
        "augment_string", (PyCFunction)string_noise_augment_string, METH_VARARGS | METH_KEYWORDS,
        "Augments a string based on the specified mapping and probability."
    },
    {NULL, NULL, 0, NULL} // Sentinel
};

// Make sure the module definition and initialization are correct
static struct PyModuleDef stringnoisemodule = {
    PyModuleDef_HEAD_INIT,
    "string_noise",
    "Module for augmenting strings with noise.",
    -1,
    StringNoiseMethods
};

PyMODINIT_FUNC PyInit_string_noise(void) {
    PyObject *module = PyModule_Create(&stringnoisemodule);
    if (module != NULL) {
        srand((unsigned int)time(NULL));  // Seed the random number generator once
    }
    return module;
}

#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// The augmented string_noise function
static PyObject* string_noise_augment_string(PyObject* self, PyObject* args, PyObject* kwds) {
    PyObject *input_string, *replacement_mapping, *output_string = NULL, *key, *value, *debug_obj = NULL;
    double probability;
    int debug = 0;  // Default debug mode is off
    Py_ssize_t i, input_len;
    char *replacements_done;

    static char* kwlist[] = {"input_string", "replacement_mapping", "probability", "debug", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "UO!d|O", kwlist,
                                     &input_string,
                                     &PyDict_Type, &replacement_mapping,
                                     &probability,
                                     &debug_obj)) {
        return NULL;
    }

    if (probability < 0.0 || probability > 1.0) {
        PyErr_SetString(PyExc_ValueError, "Probability must be between 0 and 1");
        return NULL;
    }

    if (debug_obj && PyObject_IsTrue(debug_obj)) {
        debug = 1;
    }

    // Validate the replacement mapping
    Py_ssize_t pos = 0;
    while (PyDict_Next(replacement_mapping, &pos, &key, &value)) {
        if (!PyUnicode_Check(key)) {
            PyErr_SetString(PyExc_TypeError, "All keys in the replacement mapping must be strings");
            return NULL;
        }
        if (!(PyUnicode_Check(value) || (PyList_Check(value) && PyList_Size(value) > 0))) {
            PyErr_SetString(PyExc_TypeError, "All values in the replacement mapping must be strings or non-empty lists of strings");
            return NULL;
        }
        if (PyList_Check(value)) {
            for (Py_ssize_t i = 0; i < PyList_Size(value); i++) {
                PyObject *item = PyList_GetItem(value, i);
                if (!PyUnicode_Check(item)) {
                    PyErr_SetString(PyExc_TypeError, "All items in the replacement mapping lists must be strings");
                    return NULL;
                }
            }
        }
    }

    if (debug) {
        printf("Original input string: %s\n", PyUnicode_AsUTF8(input_string));
    }

    // Step 1: Create an array to store the keys
    PyObject **sorted_keys = (PyObject **)PyMem_RawMalloc(PyDict_Size(replacement_mapping) * sizeof(PyObject *));
    if (!sorted_keys) {
        PyErr_NoMemory();
        return NULL;
    }
    Py_ssize_t key_count = 0;
    
    // Step 2: Populate the array with keys from the dictionary
    pos = 0;
    while (PyDict_Next(replacement_mapping, &pos, &key, &value)) {
        Py_INCREF(key);  // Increment reference count for key
        sorted_keys[key_count++] = key;
    }
    
    // Step 3: Sort the keys by length in descending order
    for (Py_ssize_t i = 0; i < key_count - 1; i++) {
        for (Py_ssize_t j = 0; j < key_count - i - 1; j++) {
            if (PyUnicode_GET_LENGTH(sorted_keys[j]) < PyUnicode_GET_LENGTH(sorted_keys[j + 1])) {
                PyObject *temp = sorted_keys[j];
                sorted_keys[j] = sorted_keys[j + 1];
                sorted_keys[j + 1] = temp;
            }
        }
    }

    input_len = PyUnicode_GET_LENGTH(input_string);
    replacements_done = (char *)PyMem_RawMalloc(input_len);
    if (!replacements_done) {
        PyMem_RawFree(sorted_keys);
        PyErr_NoMemory();
        return NULL;
    }
    memset(replacements_done, 0, input_len);

    output_string = PyUnicode_FromObject(input_string);
    if (!output_string) {
        PyMem_RawFree(sorted_keys);
        PyMem_RawFree(replacements_done);
        PyErr_NoMemory();
        return NULL;
    }

    for (i = 0; i < input_len; i++) {
        if (replacements_done[i]) continue;
    
        // Iterate over each key-value pair in the replacement mapping
        for (Py_ssize_t sorted_index = 0; sorted_index < key_count; sorted_index++) {
            key = sorted_keys[sorted_index];
            value = PyDict_GetItem(replacement_mapping, key);
    
            const Py_ssize_t key_len = PyUnicode_GET_LENGTH(key);
            if (i + key_len > input_len) continue;
    
            PyObject *substring = PyUnicode_Substring(input_string, i, i + key_len);
            if (!substring) {
                PyMem_RawFree(sorted_keys);
                PyMem_RawFree(replacements_done);
                Py_DECREF(output_string);
                return NULL;
            }
    
            if (debug) {
                printf("Checking substring: %s against key: %s\n", PyUnicode_AsUTF8(substring), PyUnicode_AsUTF8(key));
            }
    
            int compare_result = PyUnicode_Compare(substring, key);
            Py_DECREF(substring);
    
            if (compare_result == 0 && rand() < (RAND_MAX * probability)) {
                PyObject *replacement;
    
                if (PyList_Check(value)) {
                    Py_ssize_t replacement_list_size = PyList_Size(value);
                    int replacement_index = rand() % replacement_list_size;
                    replacement = PyList_GetItem(value, replacement_index);
                } else if (PyUnicode_Check(value)) {
                    replacement = value;
                } else {
                    // Handle unexpected data type
                    PyMem_RawFree(sorted_keys);
                    PyMem_RawFree(replacements_done);
                    Py_DECREF(output_string);
                    PyErr_SetString(PyExc_TypeError, "Replacement mapping values must be strings or lists of strings");
                    return NULL;
                }
    
                if (!replacement) {
                    PyMem_RawFree(sorted_keys);
                    PyMem_RawFree(replacements_done);
                    Py_DECREF(output_string);
                    return NULL;
                }
                if (debug) {
                    printf("Replacing '%s' with '%s' at index %zd\n", PyUnicode_AsUTF8(key), PyUnicode_AsUTF8(replacement), i);
                }
    
                // Create a new string with the specific occurrence replaced
                PyObject *left_part = PyUnicode_Substring(output_string, 0, i);
                PyObject *right_part = PyUnicode_Substring(output_string, i + key_len, input_len);
                if (!left_part || !right_part) {
                    Py_XDECREF(left_part);  // Py_XDECREF safely handles NULL
                    Py_XDECREF(right_part);
                    PyMem_RawFree(sorted_keys);
                    PyMem_RawFree(replacements_done);
                    Py_DECREF(output_string);
                    return NULL;
                }
    
                PyObject *new_output = PyUnicode_Concat(left_part, replacement);
                Py_DECREF(left_part);
                if (!new_output) {
                    Py_DECREF(right_part);
                    PyMem_RawFree(sorted_keys);
                    PyMem_RawFree(replacements_done);
                    Py_DECREF(output_string);
                    return NULL;
                }
    
                PyObject *final_output = PyUnicode_Concat(new_output, right_part);
                Py_DECREF(new_output);
                Py_DECREF(right_part);
                if (!final_output) {
                    PyMem_RawFree(sorted_keys);
                    PyMem_RawFree(replacements_done);
                    Py_DECREF(output_string);
                    return NULL;
                }
    
                Py_DECREF(output_string);
                output_string = final_output;
                input_len = PyUnicode_GET_LENGTH(output_string);  // Update input length after replacement
    
                //if (PyUnicode_GET_LENGTH(replacement) == 0 && key_len > 0) {
                //    i--; // Decrement 'i' to account for the shift in characters
                //}
    
                if (debug) {
                    printf("Output string after replacement: %s\n", PyUnicode_AsUTF8(output_string));
                }
    
                // Mark characters as replaced
                for (Py_ssize_t k = 0; k < key_len; k++) {
                    if (i + k < input_len) {
                        replacements_done[i + k] = 1;
                    }
                }
                i += key_len - 1;
                break;  // Exit the inner loop after a successful replacement
            }
        }
    }

    for (Py_ssize_t i = 0; i < key_count; i++) {
        Py_DECREF(sorted_keys[i]);  // Decrement reference count for key
    }
    PyMem_RawFree(sorted_keys);
    PyMem_RawFree(replacements_done);

    return output_string;
}



static PyMethodDef StringNoiseMethods[] = {
    {
        "augment_string", (PyCFunction)string_noise_augment_string, METH_VARARGS | METH_KEYWORDS,
        "Augments a string based on the specified mapping and probability."
    },
    {NULL, NULL, 0, NULL} // Sentinel
};

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

#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static PyObject* parse_arguments(PyObject* args, PyObject* kwds, PyObject** input_string, PyObject** replacement_mapping, double* probability, int* debug);
static int validate_replacement_mapping(PyObject* replacement_mapping);
static PyObject* select_replacement(PyObject* value, int debug);
static int weighted_choice(double* weights, Py_ssize_t weight_count);
static PyObject* normalize_string(PyObject* input_string);
static void shuffle_pyobject_array(PyObject** array, Py_ssize_t n);
static PyObject* perform_replacements(PyObject* input_string, PyObject* replacement_mapping, double probability, int debug);
static PyObject* string_noise_augment_string(PyObject* self, PyObject* args, PyObject* kwds);


// Shuffle an array of PyObject pointers
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

static int validate_replacement_mapping(PyObject* replacement_mapping) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(replacement_mapping, &pos, &key, &value)) {
        if (!PyUnicode_Check(key)) {
            PyErr_SetString(PyExc_TypeError, "All keys in the replacement mapping must be strings");
            return 0;
        }

        if (PyDict_Check(value)) {
            PyObject *subkey, *subvalue;
            Py_ssize_t subpos = 0;
            double total_weight = 0.0;

            while (PyDict_Next(value, &subpos, &subkey, &subvalue)) {
                if (!PyUnicode_Check(subkey) || !PyFloat_Check(subvalue)) {
                    PyErr_SetString(PyExc_TypeError, "In weighted replacement mappings, keys must be strings and values must be floats");
                    return 0;
                }
                total_weight += PyFloat_AsDouble(subvalue);
            }

            // Optional check for total weight
            // if(fabs(total_weight - 1.0) > some_small_value) {
            //     PyErr_SetString(PyExc_ValueError, "Total weight for each key must sum up to 1.0");
            //     return 0;
            // }
        } else if (PyList_Check(value)) {
            if (PyList_Size(value) == 0) {
                PyErr_SetString(PyExc_ValueError, "Replacement lists cannot be empty");
                return 0;
            }
            for (Py_ssize_t i = 0; i < PyList_Size(value); i++) {
                PyObject* item = PyList_GetItem(value, i);
                if (!PyUnicode_Check(item)) {
                    PyErr_SetString(PyExc_TypeError, "All elements in replacement lists must be strings");
                    return 0;
                }
            }
        } else if (!PyUnicode_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "All values in the replacement mapping must be strings, non-empty lists of strings, or dictionaries");
            return 0;
        }
    }
    return 1;
}


static int weighted_choice(double* weights, Py_ssize_t weight_count) {
    double total_weight = 0.0;
    double r = (double)rand() / RAND_MAX;
    for (Py_ssize_t i = 0; i < weight_count; i++) {
        total_weight += weights[i];
        if (r <= total_weight) {
            return i;
        }
    }
    return -1; // In case something goes wrong
}

static PyObject* normalize_string(PyObject* input_string) {
    // Convert the input string to a UTF-8 encoded byte string
    PyObject *utf8_string = PyUnicode_AsUTF8String(input_string);
    if (!utf8_string) {
        // Handle error
        return NULL;
    }

    // Convert the UTF-8 encoded byte string back to a Unicode object
    PyObject *normalized_string = PyUnicode_FromEncodedObject(utf8_string, "utf-8", "strict");
    Py_DECREF(utf8_string);  // Clean up the intermediate byte string

    if (!normalized_string) {
        // Handle error
        return NULL;
    }

    return normalized_string;
}


static PyObject* select_replacement(PyObject* value, int debug) {
    PyObject *replacement = NULL;

    if (PyDict_Check(value)) {
        // Handle dictionary replacement values.
        Py_ssize_t replacement_count = PyDict_Size(value);
        PyObject **replacement_values = (PyObject**)malloc(replacement_count * sizeof(PyObject*));
        double *replacement_weights = (double*)malloc(replacement_count * sizeof(double));
        if (!replacement_values || !replacement_weights) {
            PyErr_NoMemory();
            free(replacement_values); // Safely free in case one allocation succeeded.
            free(replacement_weights);
            return NULL;
        }

        PyObject *subkey, *subvalue;
        Py_ssize_t pos = 0;
        Py_ssize_t idx = 0;
        while (PyDict_Next(value, &pos, &subkey, &subvalue)) {
            Py_INCREF(subkey); 
            replacement_values[idx] = subkey;
            replacement_weights[idx] = PyFloat_AsDouble(subvalue);
            idx++;
        }

        int chosen_index = weighted_choice(replacement_weights, replacement_count);
        if (chosen_index < 0 || chosen_index >= replacement_count) {
            for (Py_ssize_t i = 0; i < replacement_count; i++) {
                Py_DECREF(replacement_values[i]);
            }
            free(replacement_values);
            free(replacement_weights);
            return NULL;
        }
        replacement = replacement_values[chosen_index];

        // Free memory and decrease reference count for values not chosen.
        for (Py_ssize_t i = 0; i < replacement_count; i++) {
            if (i != chosen_index) {
                Py_DECREF(replacement_values[i]);
            }
        }
        free(replacement_values);
        free(replacement_weights);
    } else if (PyList_Check(value)) {
        // Handle list replacement values.
        Py_ssize_t replacement_list_size = PyList_Size(value);
        int replacement_index = rand() % replacement_list_size;
        replacement = PyList_GetItem(value, replacement_index);
        Py_INCREF(replacement); // Increase reference count since we're returning it.

        if (debug) {
            PyObject *str = PyObject_Str(replacement);
            if (str) {
                printf("List choice replacement: %s\n", PyUnicode_AsUTF8(str));
                Py_DECREF(str);
            }
        }
    } else {
        // Direct string replacement.
        replacement = value;
        Py_INCREF(replacement); // Increase reference count since we're returning it.
    }

    return replacement;
}


static PyObject* perform_replacements(PyObject* input_string, PyObject* replacement_mapping, double probability, int debug) {
    // Get the length of the input string.
    Py_ssize_t input_len = PyUnicode_GET_LENGTH(input_string);

    // Create a new Unicode string to hold the output.
    PyObject *output_string = PyUnicode_New(input_len, 0x10FFFF);
    if (!output_string) {
        PyErr_NoMemory();
        return NULL;
    }

    // Get the keys of the replacement mapping and store them in an array.
    PyObject *keys = PyDict_Keys(replacement_mapping);
    Py_ssize_t key_count = PyList_Size(keys);
    PyObject** keys_array = (PyObject**)malloc(key_count * sizeof(PyObject*));
    if (!keys_array) {
        PyErr_NoMemory();
        Py_DECREF(output_string);
        Py_DECREF(keys);
        return NULL;
    }

    // Increment reference count for each key.
    for (Py_ssize_t i = 0; i < key_count; i++) {
        keys_array[i] = PyList_GetItem(keys, i);
        Py_INCREF(keys_array[i]);
    }

    Py_ssize_t output_len = 0;
    int error_occurred = 0;

    // Iterate over each character of the input string.
    for (Py_ssize_t i = 0; i < input_len; i++) {
        // Decide randomly whether to replace the current character.
        if ((double)rand() / RAND_MAX > probability) {
            // If not replacing, copy the current character to the output string.
            Py_UCS4 ch = PyUnicode_READ_CHAR(input_string, i);
            PyUnicode_WRITE(PyUnicode_KIND(output_string), PyUnicode_DATA(output_string), output_len++, ch);
            continue;
        }

        // Shuffle the keys array for random replacement selection.
        shuffle_pyobject_array(keys_array, key_count);

        // Flags to check if replacement happened.
        int replaced = 0;

        // Try to find a match for replacement.
        for (Py_ssize_t j = 0; j < key_count; j++) {
            PyObject *key = keys_array[j];
            PyObject *value = PyDict_GetItem(replacement_mapping, key);
            Py_ssize_t key_len = PyUnicode_GET_LENGTH(key);

            // Skip if the key is longer than the remaining input string.
            if (i + key_len > input_len) continue;

            // Get the substring from the input string that matches the current key's length.
            PyObject *substring = PyUnicode_Substring(input_string, i, i + key_len);
            if (!substring) {
                error_occurred = 1;
                break;
            }

            // Compare the substring with the current key.
            int compare_result = PyUnicode_Compare(substring, key);
            Py_DECREF(substring);

            // If the substring matches the key, process the replacement.
            if (compare_result == 0) {
                // Use the new function to select the replacement.
                PyObject *replacement = select_replacement(value, debug);

                // Check if replacement was successfully obtained.
                if (!replacement) {
                    error_occurred = 1;
                    break;
                }

                // Write the replacement to the output string.
                Py_ssize_t repl_len = PyUnicode_GET_LENGTH(replacement);
                for (Py_ssize_t k = 0; k < repl_len; k++) {
                    Py_UCS4 ch = PyUnicode_READ_CHAR(replacement, k);
                    if (debug) {
                        printf("Writing character: %x (Unicode code point)\n", ch);
                    }
                    PyUnicode_WRITE(PyUnicode_KIND(output_string), PyUnicode_DATA(output_string), output_len++, ch);
                }

                Py_DECREF(replacement);

                // Move the input string index forward and mark as replaced.
                i += key_len - 1;
                replaced = 1;
                break;
            }
        }

        // If an error occurred, exit the loop.
        if (error_occurred) break;

        // If no replacement occurred, copy the current character to the output string.
        if (!replaced) {
            Py_UCS4 ch = PyUnicode_READ_CHAR(input_string, i);
            PyUnicode_WRITE(PyUnicode_KIND(output_string), PyUnicode_DATA(output_string), output_len++, ch);
        }
    }

    // Clean up the keys array.
    for (Py_ssize_t i = 0; i < key_count; i++) {
        Py_DECREF(keys_array[i]);
    }
    free(keys_array);
    Py_DECREF(keys);

    // If an error occurred, clean up and return NULL.
    if (error_occurred) {
        Py_XDECREF(output_string);
        return NULL;
    }

    // Resize the output string to the actual length of the written characters.
    if (PyUnicode_Resize(&output_string, output_len) < 0) {
        Py_DECREF(output_string);
        return NULL;
    }

    // Normalize the output string.
    PyObject *normalized_output = normalize_string(output_string);
    Py_DECREF(output_string);  // Clean up the original output string.

    // If normalization failed, return NULL.
    if (!normalized_output) {
        return NULL;
    }

    // Return the normalized output string.
    return normalized_output;
}


// Main function to augment string
static PyObject* string_noise_augment_string(PyObject* self, PyObject* args, PyObject* kwds) {
    PyObject *input_string, *replacement_mapping;
    double probability;
    int debug = 0;

    // Parse arguments
    if (parse_arguments(args, kwds, &input_string, &replacement_mapping, &probability, &debug) == NULL) {
        return NULL;
    }

    // Validate replacement mapping
    if (!validate_replacement_mapping(replacement_mapping)) {
        return NULL;
    }

    // Perform replacements
    PyObject *output_string = perform_replacements(input_string, replacement_mapping, probability, debug);
    if (!output_string) {
        return NULL;
    }

    return output_string;
}

// Method definitions
static PyMethodDef StringNoiseMethods[] = {
    {
        "augment_string", (PyCFunction)string_noise_augment_string, METH_VARARGS | METH_KEYWORDS,
        "Augments a string based on the specified mapping and probability."
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
    if (module != NULL) {
        srand((unsigned int)time(NULL));  // Seed the random number generator
    }
    return module;
}

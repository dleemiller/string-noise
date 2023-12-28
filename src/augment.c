#include <Python.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "utils.h"


static int compare_pyobject_ascending(const void *a, const void *b) {
    return PyObject_RichCompareBool(*(PyObject**)a, *(PyObject**)b, Py_LT);
}

static int compare_pyobject_descending(const void *a, const void *b) {
    return PyObject_RichCompareBool(*(PyObject**)a, *(PyObject**)b, Py_GT);
}

// Shuffle an array of PyObject pointers
static void shuffle_pyobject_array(PyObject** array, Py_ssize_t n) {
    for (Py_ssize_t i = n - 1; i > 0; i--) {
        Py_ssize_t j = rand() % (i + 1);
        PyObject* temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}


static int parse_arguments(PyObject* args, PyObject* kwds, PyObject** input_string, PyObject** replacement_mapping, double* probability, int* debug, int* sort_order, int* seed) {
    PyObject* debug_obj = NULL;
    *probability = 1.0;
    static char* kwlist[] = {"input_string", "replacement_mapping", "probability", "debug", "sort_order", "seed", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "UO!|dOii", kwlist,
                                     input_string,
                                     &PyDict_Type, replacement_mapping,
                                     probability,
                                     &debug_obj,
                                     sort_order,
                                     seed)) {
        PyErr_SetString(PyExc_TypeError, "Invalid arguments: expected (string, dict, optional float, optional bool, optional int, optional int for seed)");
        return 0;
    }

    if (*probability < 0.0 || *probability > 1.0) {
        PyErr_SetString(PyExc_ValueError, "Probability must be between 0 and 1");
        return 0;
    }

    if (debug_obj && PyObject_IsTrue(debug_obj)) {
        *debug = 1;
    } else {
        *debug = 0;
    }

    return 1;
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


// Helper function to free resources
static void free_resources(PyObject* keys, PyObject** keys_array, Py_ssize_t key_count) {
    if (keys_array) {
        for (Py_ssize_t i = 0; i < key_count; i++) {
            Py_XDECREF(keys_array[i]);
        }
        free(keys_array);
    }
    Py_XDECREF(keys);
}

static PyObject* perform_replacements(PyObject* input_string, PyObject* replacement_mapping, double probability, int debug, int sort_order) {
    PyObject *keys = NULL, *normalized_output = NULL;
    PyObject **keys_array = NULL;
    Py_ssize_t key_count = 0, output_len = 0;
    int error_occurred = 0;

    Py_ssize_t input_len = PyUnicode_GET_LENGTH(input_string);
    const Py_ssize_t buffer_margin = 64;

    // Use the utility function to get aligned size
    Py_ssize_t aligned_input_len = get_aligned_size(input_len);

    PyObject *output_string = PyUnicode_New(aligned_input_len, 0x10FFFF);
    if (!output_string) goto error;

    keys = PyDict_Keys(replacement_mapping);
    key_count = PyList_Size(keys);
    keys_array = (PyObject**)malloc(key_count * sizeof(PyObject*));
    if (!keys_array) goto error;

    for (Py_ssize_t i = 0; i < key_count; i++) {
        keys_array[i] = PyList_GetItem(keys, i);
        Py_INCREF(keys_array[i]);
    }

    // Sort or shuffle the keys_array based on sort_order
    if (sort_order == ASCENDING) {
        if (debug) {
            printf("SORT ASCENDING...\n");
        }
        qsort(keys_array, key_count, sizeof(PyObject*), compare_pyobject_ascending);
    } else if (sort_order == DESCENDING) {
        if (debug) {
            printf("SORT DESCENDING...\n");
        }
        qsort(keys_array, key_count, sizeof(PyObject*), compare_pyobject_descending);
    } else if (sort_order == SHUFFLE) { // SHUFFLE
        if (debug) {
            printf("SORT SHUFFLE...\n");
        }
        shuffle_pyobject_array(keys_array, key_count);
    }

    for (Py_ssize_t i = 0; i < input_len; i++) {
        if ((double)rand() / RAND_MAX > probability) {
            Py_UCS4 ch = PyUnicode_READ_CHAR(input_string, i);
            if (!write_char_to_output(&output_string, &output_len, ch, buffer_margin, debug)) {
                goto error;
            }
            continue;
        }
        if (sort_order == RESHUFFLE) {
            if (debug) {
                printf("SORT RESHUFFLE...\n");
            }
            shuffle_pyobject_array(keys_array, key_count);
        }
        int replaced = 0;

        for (Py_ssize_t j = 0; j < key_count; j++) {
            PyObject *key = keys_array[j];
            if (debug) {
                PyObject *key_repr = PyObject_Str(key);  // Convert the PyObject to a string representation
                if (key_repr != NULL) {
                    const char *key_cstr = PyUnicode_AsUTF8(key_repr);  // Convert the Python string to a C string
                    if (key_cstr != NULL) {
                        printf("Key: %s\n", key_cstr);
                    } else {
                        PyErr_Print();  // Handle potential error in conversion
                    }
                    Py_DECREF(key_repr);  // Clean up the temporary Python string object
                } else {
                    PyErr_Print();  // Handle potential error in conversion
                }
            }

            PyObject *value = PyDict_GetItem(replacement_mapping, key);
            Py_ssize_t key_len = PyUnicode_GET_LENGTH(key);

            if (i + key_len > input_len) continue;

            PyObject *substring = PyUnicode_Substring(input_string, i, i + key_len);
            if (!substring) {
                error_occurred = 1;
                break;
            }

            int compare_result = PyUnicode_Compare(substring, key);
            Py_DECREF(substring);

            if (compare_result == 0) {
                PyObject *replacement = select_replacement(value, debug);
                if (!replacement) {
                    error_occurred = 1;
                    break;
                }

                Py_ssize_t repl_len = PyUnicode_GET_LENGTH(replacement);

                for (Py_ssize_t k = 0; k < repl_len; k++) {
                    Py_UCS4 ch = PyUnicode_READ_CHAR(replacement, k);
                    if (!write_char_to_output(&output_string, &output_len, ch, buffer_margin, debug)) {
                        Py_DECREF(replacement);
                        goto error;
                    }
                }

                Py_DECREF(replacement);
                i += key_len - 1;
                replaced = 1;
                break;
            }
        }

        if (error_occurred) break;

        if (!replaced) {
            Py_UCS4 ch = PyUnicode_READ_CHAR(input_string, i);
            if (!write_char_to_output(&output_string, &output_len, ch, buffer_margin, debug)) {
                goto error;
            }
        }
    }

    if (error_occurred) goto error;

    // Debug print: Final output string length and contents
    if (debug && output_string) {
        printf("Debug: Final output string length: %zd\n", PyUnicode_GET_LENGTH(output_string));
        printf("Debug: Final output string: %s\n", PyUnicode_AsUTF8(output_string));
    }

    // Resize output string to match output_len
    if (PyUnicode_Resize(&output_string, output_len) < 0) {
        Py_DECREF(output_string);
        return PyErr_NoMemory();
    }

    normalized_output = normalize_string(output_string);
    if (!normalized_output) goto error;

    goto final;

error:
    Py_XDECREF(output_string);
    Py_XDECREF(normalized_output);
    free_resources(keys, keys_array, key_count);
    return NULL;

final:
    Py_XDECREF(output_string);
    free_resources(keys, keys_array, key_count);
    return normalized_output;
}

PyObject* augment_string(PyObject* self, PyObject* args, PyObject* kwds) {
    PyObject *input_string, *replacement_mapping;
    double probability;
    int debug = 0, sort_order = RESHUFFLE, seed = -1; // Add seed parameter with default value -1

    if (!parse_arguments(args, kwds, &input_string, &replacement_mapping, &probability, &debug, &sort_order, &seed)) {
        return NULL;
    }

    // Seed the random number generator
    if (seed == -1) {
        srand((unsigned int)clock());
    } else {
        srand((unsigned int)seed);
    }

    if (!validate_replacement_mapping(replacement_mapping)) {
        return NULL;
    }

    PyObject *output_string = perform_replacements(input_string, replacement_mapping, probability, debug, sort_order);
    if (!output_string) {
        PyErr_SetString(PyExc_RuntimeError, "String augmentation failed.");
        return NULL;
    }

    return output_string;
}

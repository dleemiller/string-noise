#include <Python.h>
#include <stdio.h>
#include "normalize.h"

static PyObject* convert_and_normalize(PyObject* input, int debug) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    double total = 0.0;
    PyObject* dict;

    if (debug) {
        printf("Debug: Starting normalization process.\n");
    }

    if (PyList_Check(input)) {
        if (debug) {
            printf("Debug: Input is a list.\n");
        }
        Py_ssize_t size = PyList_Size(input);
        dict = PyDict_New();
        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject* item = PyList_GetItem(input, i);
            PyDict_SetItem(dict, item, PyFloat_FromDouble(1.0 / size));
        }
    } else if (PyDict_Check(input)) {
        if (debug) {
            printf("Debug: Input is a dictionary.\n");
        }
        dict = PyDict_New();
        while (PyDict_Next(input, &pos, &key, &value)) {
            if (PyUnicode_Check(value)) {
                PyObject* new_dict = PyDict_New();
                PyDict_SetItem(new_dict, value, PyFloat_FromDouble(1.0));
                PyDict_SetItem(dict, key, new_dict);
                Py_DECREF(new_dict);
            } else {
                PyDict_SetItem(dict, key, value);
                Py_INCREF(value);
            }
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "Input must be a list or a dictionary");
        return NULL;
    }

    pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
        double val = PyFloat_AsDouble(value);
        if (val < 0) {
            PyErr_SetString(PyExc_ValueError, "Negative values are not allowed");
            return NULL;
        }
        total += val;
    }

    if (debug) {
        printf("Debug: Total sum of weights = %f\n", total);
    }

    if (total == 0) {
        PyErr_SetString(PyExc_ValueError, "Total sum of weights is zero, cannot normalize");
        return NULL;
    }

    pos = 0;
    if (debug) {
        printf("Debug: Starting normalization of each value.\n");
    }
    while (PyDict_Next(dict, &pos, &key, &value)) {
        double normalized_val = PyFloat_AsDouble(value) / total;
        PyObject* new_value = PyFloat_FromDouble(normalized_val);
        PyDict_SetItem(dict, key, new_value);
        Py_DECREF(new_value);
        if (debug) {
            printf("Debug: Normalizing key %s, value %f to %f\n", 
                   PyUnicode_AsUTF8(key), PyFloat_AsDouble(value), normalized_val);
        }
    }

    if (debug) {
        printf("Debug: Normalization process complete.\n");
    }

    return dict;
}

PyObject* normalize(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* input_dict;
    static char* kwlist[] = {"input", "debug", NULL};
    int debug = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|p", kwlist, &input_dict, &debug)) {
        return NULL;
    }

    if (!PyDict_Check(input_dict)) {
        PyErr_SetString(PyExc_TypeError, "Input must be a dictionary");
        return NULL;
    }

    PyObject *key, *value;
    Py_ssize_t pos = 0;
    PyObject* normalized_dict = PyDict_New();

    while (PyDict_Next(input_dict, &pos, &key, &value)) {
        PyObject* normalized_value;

        if (PyDict_Check(value) || PyList_Check(value)) {
            normalized_value = convert_and_normalize(value, debug);
        } else if (PyUnicode_Check(value)) {
            normalized_value = PyDict_New();
            PyDict_SetItem(normalized_value, value, PyFloat_FromDouble(1.0));
        } else {
            normalized_value = value;
            Py_INCREF(normalized_value);
        }

        if (!normalized_value) {
            Py_DECREF(normalized_dict);
            return NULL;
        }

        PyDict_SetItem(normalized_dict, key, normalized_value);
        Py_DECREF(normalized_value);
    }

    return normalized_dict;
}


#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"


TrieNode* createNode(void) {
    TrieNode *newNode = malloc(sizeof(TrieNode));
    if (newNode == NULL) {
        return NULL; // Handle memory allocation failure
    }
    newNode->isEndOfWord = 0;
    newNode->mapping = NULL; // Initialize mapping to NULL
    for (int i = 0; i < TRIE_NODE_SIZE; i++) {
        newNode->children[i] = NULL;
    }
    return newNode;
}

int charToIndex(char c) {
    unsigned char uc = (unsigned char)c;
    return (int)uc;
}

void insertIntoTrie(TrieNode *root, const char *word, PyObject *mapping) {
    TrieNode *current = root;
    while (*word) {
        int index = charToIndex(*word);
        if (index == -1) {
            // Handle invalid character
            return;
        }
        if (!current->children[index]) {
            current->children[index] = createNode();
        }
        current = current->children[index];
        word++;
    }
    current->isEndOfWord = 1;
    current->mapping = mapping; // Attach the mapping list to the node
}



void freeTrie(TrieNode *root) {
    if (root == NULL) return;
    for (int i = 0; i < TRIE_NODE_SIZE; i++) {
        if (root->children[i]) {
            freeTrie(root->children[i]);
        }
    }
    if (root->mapping != NULL) {
        Py_DECREF(root->mapping); // Decrement reference count of the mapping list
    }
    free(root);
}


PyObject* build_tree(PyObject *self, PyObject *args) {
    PyObject *py_dict;
    if (!PyArg_ParseTuple(args, "O", &py_dict)) {
        return NULL; // Argument parsing failed
    }
    if (!PyDict_Check(py_dict)) {
        PyErr_SetString(PyExc_TypeError, "Argument must be a dictionary");
        return NULL;
    }

    // Create a new PyTrieObject instance
    PyTrieObject *pyTrie = PyObject_New(PyTrieObject, &PyTrieType);
    if (!pyTrie) {
        PyErr_SetString(PyExc_MemoryError, "Failed to create Trie object");
        return NULL;
    }

    // Initialize the trie root for the new object
    pyTrie->root = createNode();
    if (pyTrie->root == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory for the Trie root");
        Py_DECREF(pyTrie);
        return NULL;
    }

    // Populate the trie
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(py_dict, &pos, &key, &value)) {
        if (!PyUnicode_Check(key)) {
            PyErr_SetString(PyExc_TypeError, "All keys in the dictionary must be strings");
            Py_DECREF(pyTrie);
            return NULL;
        }

        // Check for string value
        if (PyUnicode_Check(value)) {
            Py_INCREF(value);
        }
        // Check for list of strings
        else if (PyList_Check(value)) {
            for (Py_ssize_t i = 0; i < PyList_Size(value); i++) {
                PyObject *item = PyList_GetItem(value, i);
                if (!PyUnicode_Check(item)) {
                    PyErr_SetString(PyExc_TypeError, "All elements in the list must be strings");
                    Py_DECREF(pyTrie);
                    return NULL;
                }
            }
            Py_INCREF(value);
        }
        // Check for dict with string keys and float values
        else if (PyDict_Check(value)) {
            PyObject *sub_key, *sub_value;
            Py_ssize_t sub_pos = 0;
            while (PyDict_Next(value, &sub_pos, &sub_key, &sub_value)) {
                if (!PyUnicode_Check(sub_key) || !PyFloat_Check(sub_value)) {
                    PyErr_SetString(PyExc_TypeError, "In a dict value, all keys must be strings and all values must be floats");
                    Py_DECREF(pyTrie);
                    return NULL;
                }
            }
            Py_INCREF(value);
        }
        // If none of the above, raise an error
        else {
            PyErr_SetString(PyExc_TypeError, "Values must be a string, a list of strings, or a dict with string keys and float values");
            Py_DECREF(pyTrie);
            return NULL;
        }

        const char *word = PyUnicode_AsUTF8(key);
        insertIntoTrie(pyTrie->root, word, value);
    }

    return (PyObject *)pyTrie;
}

TrieNode* lookupInTrie(TrieNode *root, const char *word) {
    TrieNode *current = root;
    while (*word) {
        int index = charToIndex(*word);
        if (index == -1 || !current->children[index]) {
            return NULL;
        }
        current = current->children[index];
        word++;
    }
    return (current != NULL && current->isEndOfWord) ? current : NULL;
}


PyObject* PyTrie_lookup(PyTrieObject *self, PyObject *args) {
    const char *word;
    if (!PyArg_ParseTuple(args, "s", &word)) {
        return NULL; // Argument parsing failed
    }

    TrieNode *current = self->root;
    while (*word) {
        int index = charToIndex(*word);
        if (index == -1 || !current->children[index]) {
            Py_RETURN_NONE;
        }
        current = current->children[index];
        word++;
    }

    if (current != NULL && current->isEndOfWord) {
        if (current->mapping != NULL) {
            Py_INCREF(current->mapping); // Increment ref count before returning
            return current->mapping;
        }
    }

    Py_RETURN_NONE; // Return None if word not found or no mapping
}

// Define methods of PyTrie type
static PyMethodDef PyTrie_methods[] = {
    {"lookup", (PyCFunction)PyTrie_lookup, METH_VARARGS, "Look up a word in the trie."},
    {NULL}  /* Sentinel */
};

void PyTrie_dealloc(PyTrieObject *self) {
    freeTrie(self->root);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyTypeObject PyTrieType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "string_noise.Trie",
    .tp_doc = "Trie object for string noise",
    .tp_basicsize = sizeof(PyTrieObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = PyTrie_methods,
    .tp_dealloc = (destructor) PyTrie_dealloc,
    .tp_new = PyType_GenericNew,
};


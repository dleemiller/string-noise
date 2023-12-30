#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"

TrieNode *trieRoot = NULL;


TrieNode* createNode(void) {
    TrieNode *newNode = malloc(sizeof(TrieNode));
    if (newNode == NULL) {
        return NULL; // Handle memory allocation failure
    }
    newNode->isEndOfWord = 0;
    newNode->misspellings = NULL; // Initialize misspellings to NULL
    for (int i = 0; i < TRIE_NODE_SIZE; i++) {
        newNode->children[i] = NULL;
    }
    return newNode;
}

int charToIndex(char c) {
    unsigned char uc = (unsigned char)c;
    return (int)uc;
}

void insertIntoTrie(TrieNode *root, const char *word, PyObject *misspellings) {
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
    current->misspellings = misspellings; // Attach the misspellings list to the node
}



void freeTrie(TrieNode *root) {
    if (root == NULL) return;
    for (int i = 0; i < TRIE_NODE_SIZE; i++) {
        if (root->children[i]) {
            freeTrie(root->children[i]);
        }
    }
    if (root->misspellings != NULL) {
        Py_DECREF(root->misspellings); // Decrement reference count of the misspellings list
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
        if (!PyUnicode_Check(key) || !PyList_Check(value)) {
            continue; // Skip invalid entries
        }

        Py_INCREF(value); // Increment reference count of the misspellings list
        const char *correct_spelling = PyUnicode_AsUTF8(key);
        insertIntoTrie(pyTrie->root, correct_spelling, value);
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
        if (current->misspellings != NULL) {
            Py_INCREF(current->misspellings); // Increment ref count before returning
            return current->misspellings;
        }
    }

    Py_RETURN_NONE; // Return None if word not found or no misspellings
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


#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include "mispelling.h"

TrieNode *trieRoot = NULL;


TrieNode* createNode(void) {
    TrieNode *newNode = malloc(sizeof(TrieNode));
    if (newNode == NULL) {
        return NULL; // Handle memory allocation failure
    }
    newNode->isEndOfWord = 0;
    newNode->misspellings = NULL; // Initialize misspellings to NULL
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        newNode->children[i] = NULL;
    }
    return newNode;
}

int charToIndex(char c) {
    if (isalpha(c)) {
        return tolower(c) - 'a';  // Map letters to indices 0-25
    } else if (isdigit(c)) {
        return 26 + (c - '0');    // Map digits to indices 26-35
    } else {
        // Map punctuation to indices 36-50
        switch (c) {
            case '.': return 36;
            case ',': return 37;
            case '\'': return 38;
            case '!': return 39;
            case '?': return 40;
            case ';': return 41;
            case ':': return 42;
            case '-': return 43;
            case '_': return 44;
            case '"': return 45;
            case '(': return 46;
            case ')': return 47;
            case '[': return 48;
            case ']': return 49;
            case '{': return 50;
            case '}': return 51;
            // Extend with additional punctuation if needed
            default: return -1; // Invalid character
        }
    }
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
    for (int i = 0; i < ALPHABET_SIZE; i++) {
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

    // Free the existing trie
    if (trieRoot != NULL) {
        freeTrie(trieRoot);
        trieRoot = NULL;
    }

    trieRoot = createNode();
    if (trieRoot == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory for the Trie root");
        return NULL;
    }

    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(py_dict, &pos, &key, &value)) {
        if (!PyUnicode_Check(key) || !PyList_Check(value)) {
            continue; // Skip invalid entries
        }

        Py_INCREF(value); // Increment reference count of the misspellings list
        const char *correct_spelling = PyUnicode_AsUTF8(key);
        insertIntoTrie(trieRoot, correct_spelling, value); // Pass the misspellings list
    }

    Py_RETURN_NONE;
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


PyObject* lookup(PyObject *self, PyObject *args) {
    const char *word;
    if (!PyArg_ParseTuple(args, "s", &word)) {
        return NULL; // Argument parsing failed
    }

    TrieNode *current = trieRoot;
    while (*word) {
        if (current == NULL) {
            Py_RETURN_NONE;
        }
        int index = tolower(*word) - 'a';
        current = current->children[index];
        word++;
    }

    if (current != NULL && current->isEndOfWord && current->misspellings != NULL) {
        Py_INCREF(current->misspellings); // Increment ref count before returning
        return current->misspellings;
    }

    Py_RETURN_NONE; // Return None if word not found or no misspellings
}


//// Function to define methods of the module
//static PyMethodDef MispellingMethods[] = {
//    {"build_tree",  build_tree, METH_VARARGS, "Build the Trie from a dictionary of misspellings."},
//    {NULL, NULL, 0, NULL} // Sentinel
//};
//
//// Module definition
//static struct PyModuleDef mispellingmodule = {
//    PyModuleDef_HEAD_INIT,
//    "mispelling", // name of module
//    NULL, // module documentation
//    -1, // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
//    MispellingMethods
//};
//
//// Initialization function for the module
//PyMODINIT_FUNC PyInit_mispelling(void) {
//    return PyModule_Create(&mispellingmodule);
//}
//

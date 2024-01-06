#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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


MarkovNode* createMarkovNode(void) {
    MarkovNode *newNode = malloc(sizeof(MarkovNode));
    if (newNode == NULL) {
        return NULL; // Handle memory allocation failure
    }
    memset(newNode->characterCounts, 0, sizeof(newNode->characterCounts)); // Initialize counts to 0
    for (int i = 0; i < TRIE_NODE_SIZE; i++) {
        newNode->children[i] = NULL;
    }
    return newNode;
}

void freeMarkovTrie(MarkovNode *root) {
    if (root == NULL) return;
    for (int i = 0; i < TRIE_NODE_SIZE; i++) {
        if (root->children[i]) {
            freeMarkovTrie(root->children[i]);
        }
    }
    free(root);
}


static void PyMarkovTrie_dealloc(PyMarkovTrieObject *self) {
    freeMarkovTrie(self->forwardRoot);
    freeMarkovTrie(self->reverseRoot);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

MarkovNode* getOrCreateChild(MarkovNode *node, char character, int debug) {
    if (debug) {
        printf("getOrCreateChild: character = %c\n", character);
    }
    if (node == NULL) {
        printf("Error: Received NULL node in getOrCreateChild\n");
        return NULL;
    }

    if (debug) {
        printf("Accessing child for character %c\n", character);
    }
    if (node->children[(unsigned char)character] == NULL) {
        if (debug) {
            printf("Creating new child node for character %c\n", character);
        }
        node->children[(unsigned char)character] = createMarkovNode();
        if (node->children[(unsigned char)character] == NULL) {
            if (debug) {
                printf("Failed to create child node for character %c\n", character);
            }
            return NULL;
        }
    }
    return node->children[(unsigned char)character];
}

// Utility function to increment character count
void incrementCharacterCount(MarkovNode *node, char character) {
    node->characterCounts[(unsigned char)character]++;
}

// Helper function to check if a char is a multi-byte UTF-8 character
bool isMultiByteChar(char c) {
    // Check if the most significant bit is set (indicative of multi-byte characters in UTF-8)
    return (c & 0x80) != 0;
}

static PyObject* index_string_MarkovTrie(PyMarkovTrieObject *self, PyObject *args) {
    const char *inputString;
    int debug = 0;  // Default value for debug flag

    if (!PyArg_ParseTuple(args, "s|p", &inputString, &debug)) {
        return NULL; // Error in parsing arguments
    }

    if (debug) {
        printf("Indexing string: %s\n", inputString);
    }

    size_t length = strlen(inputString);
    if (length < 3) {
        if (debug) {
            printf("String too short for trigram processing. Length: %zu\n", length);
        }
        return PyLong_FromLong(0);
    }

    int trigramCount = 0;
    for (size_t i = 0; i < length - 2; i++) {
        if (isMultiByteChar(inputString[i]) || 
            isMultiByteChar(inputString[i + 1]) || 
            isMultiByteChar(inputString[i + 2])) {
            continue; // Skip trigrams with multi-byte characters
        }

        if (debug) {
            printf("Processing forward trigram: %c%c%c\n", inputString[i], inputString[i + 1], inputString[i + 2]);
        }

        // Forward indexing
        MarkovNode *firstNode = getOrCreateChild(self->forwardRoot, inputString[i], debug);
        if (firstNode == NULL) return NULL;
        MarkovNode *secondNode = getOrCreateChild(firstNode, inputString[i + 1], debug);
        if (secondNode == NULL) return NULL;
        incrementCharacterCount(secondNode, inputString[i + 2]);

        // Process the reverse trigram starting from the end of the string
        if (debug) {
            printf("Processing reverse trigram: %c%c%c\n", inputString[length - i - 1], inputString[length - i - 2], inputString[length - i - 3]);
        }

        // Reverse indexing
        MarkovNode *lastNode = getOrCreateChild(self->reverseRoot, inputString[length - i - 1], debug);
        if (lastNode == NULL) return NULL;
        MarkovNode *secondLastNode = getOrCreateChild(lastNode, inputString[length - i - 2], debug);
        if (secondLastNode == NULL) return NULL;
        incrementCharacterCount(secondLastNode, inputString[length - i - 3]);

        trigramCount++;
    }

    if (debug) {
        printf("Indexed %d trigrams\n", trigramCount);
    }
    return PyLong_FromLong(trigramCount);
}

void traverseMarkovNode(MarkovNode *node, PyObject *dict, int depth, int debug) {
    if (node == NULL || depth == 0) return;

    for (int i = 0; i < 256; i++) {
        if (i > 127) break;

        if (node->children[i] != NULL) {
            PyObject *childDict = PyDict_New();
            traverseMarkovNode(node->children[i], childDict, depth - 1, debug);

            char key[2] = {(char)i, '\0'};
            if (debug) {
                printf("Adding child dict for ASCII character: %d (%s)\n", i, key);
            }
            PyDict_SetItemString(dict, key, childDict);
            Py_DECREF(childDict);
        }

        if (node->characterCounts[i] > 0) {
            char key[2] = {(char)i, '\0'};
            PyObject *count = PyLong_FromLong(node->characterCounts[i]);
            if (debug) {
                printf("Setting count for ASCII character: %d (%s), count: %ld\n", i, key, PyLong_AsLong(count));
            }
            PyDict_SetItemString(dict, key, count);
            Py_DECREF(count);
        }
    }
}

static PyObject* dump_MarkovTrie(PyMarkovTrieObject *self, PyObject *args) {
    int debug = 0;  // Default value for debug flag

    // Parse the optional debug argument
    if (!PyArg_ParseTuple(args, "|p", &debug)) {
        return NULL; // Error in parsing arguments
    }

    PyObject *result = PyDict_New();
    PyObject *forwardDict = PyDict_New();
    PyObject *reverseDict = PyDict_New();

    // Traverse for forward and reverse mappings with debug flag
    if (self->forwardRoot != NULL) {
        traverseMarkovNode(self->forwardRoot, forwardDict, MAX_DEPTH, debug);
    }
    if (self->reverseRoot != NULL) {
        traverseMarkovNode(self->reverseRoot, reverseDict, MAX_DEPTH, debug);
    }

    PyDict_SetItemString(result, "forward", forwardDict);
    PyDict_SetItemString(result, "reverse", reverseDict);

    Py_DECREF(forwardDict);
    Py_DECREF(reverseDict);

    return result;
}

bool validateSubTrieDict(PyObject *subDict) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(subDict, &pos, &key, &value)) {
        if (!PyUnicode_Check(key) || PyUnicode_GetLength(key) != 1) {
            PyErr_SetString(PyExc_TypeError, "Keys must be single-character strings");
            return false;
        }

        if (PyDict_Check(value)) {
            if (!validateSubTrieDict(value)) return false;
        } else if (!PyLong_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "Values must be integers or nested dictionaries");
            return false;
        }
    }
    return true;
}

bool validateTrieDict(PyObject *dict) {
    if (!PyDict_Check(dict)) {
        PyErr_SetString(PyExc_TypeError, "Expected a dictionary");
        return false;
    }

    PyObject *forwardDict = PyDict_GetItemString(dict, "forward");
    PyObject *reverseDict = PyDict_GetItemString(dict, "reverse");
    if (!forwardDict || !reverseDict || !PyDict_Check(forwardDict) || !PyDict_Check(reverseDict)) {
        PyErr_SetString(PyExc_TypeError, "Invalid format: missing or incorrect 'forward'/'reverse' dictionaries");
        return false;
    }

    return validateSubTrieDict(forwardDict) && validateSubTrieDict(reverseDict);
}

bool loadTrieFromDict(MarkovNode *node, PyObject *dict) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(dict, &pos, &key, &value)) {
        // Ensure the key is a string
        if (!PyUnicode_Check(key)) {
            PyErr_SetString(PyExc_TypeError, "Trie keys must be strings");
            return false;
        }

        // Convert key to char
        const char *keyStr = PyUnicode_AsUTF8(key);
        if (!keyStr || strlen(keyStr) != 1) {
            PyErr_SetString(PyExc_ValueError, "Trie keys must be single characters");
            return false;
        }
        char character = keyStr[0];

        // If value is a dictionary, recurse
        if (PyDict_Check(value)) {
            node->children[(unsigned char)character] = createMarkovNode();
            if (!node->children[(unsigned char)character] || !loadTrieFromDict(node->children[(unsigned char)character], value)) {
                return false;
            }
        }
        // If value is an integer, set the character count
        else if (PyLong_Check(value)) {
            node->characterCounts[(unsigned char)character] = PyLong_AsLong(value);
        } else {
            PyErr_SetString(PyExc_TypeError, "Trie values must be integers or dictionaries");
            return false;
        }
    }
    return true;
}

static PyObject* load_MarkovTrie(PyMarkovTrieObject *self, PyObject *args) {
    PyObject *trieDict;

    // Parse the input dictionary
    if (!PyArg_ParseTuple(args, "O", &trieDict)) {
        return NULL; // Error in parsing arguments
    }

    // Validate the input format
    if (!validateTrieDict(trieDict)) {
        // Error message set in validateTrieDict
        return NULL;
    }

    // Extract forward and reverse dictionaries
    PyObject *forwardDict = PyDict_GetItemString(trieDict, "forward");
    PyObject *reverseDict = PyDict_GetItemString(trieDict, "reverse");
    if (!forwardDict || !reverseDict) {
        PyErr_SetString(PyExc_KeyError, "Dictionary must contain 'forward' and 'reverse' keys");
        return NULL;
    }

    // Clear existing Trie before loading new data
    freeMarkovTrie(self->forwardRoot);
    self->forwardRoot = createMarkovNode();
    freeMarkovTrie(self->reverseRoot);
    self->reverseRoot = createMarkovNode();

    // Load forward Trie
    if (!loadTrieFromDict(self->forwardRoot, forwardDict)) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to load forward Trie");
        return NULL;
    }

    // Load reverse Trie
    if (!loadTrieFromDict(self->reverseRoot, reverseDict)) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to load reverse Trie");
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyMethodDef PyMarkovTrie_methods[] = {
    // Method definitions go here
    // Example: {"load", (pycfunction)pymarkovtrie_load, meth_varargs, "load a markov trie from a python object."},
    {"index_string", (PyCFunction)index_string_MarkovTrie, METH_VARARGS, "Index a string in the Markov Trie."},
    {"dump", (PyCFunction)dump_MarkovTrie, METH_VARARGS, "Dump the Markov Trie into a Python dictionary."},
    {"load", (PyCFunction)load_MarkovTrie, METH_VARARGS, "Load data into the Markov Trie from a dictionary."},
    // Other methods: dump, replace, index, etc.
    {NULL} // Sentinel
};

static PyObject *PyMarkovTrie_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyMarkovTrieObject *self;
    self = (PyMarkovTrieObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        // Initialize forwardRoot
        self->forwardRoot = createMarkovNode();
        if (self->forwardRoot == NULL) {
            Py_DECREF(self);
            PyErr_SetString(PyExc_MemoryError, "Failed to create forward root node");
            return NULL;
        }

        // Initialize reverseRoot
        self->reverseRoot = createMarkovNode();
        if (self->reverseRoot == NULL) {
            Py_DECREF(self->forwardRoot);
            Py_DECREF(self);
            PyErr_SetString(PyExc_MemoryError, "Failed to create reverse root node");
            return NULL;
        }
    }
    return (PyObject *)self;
}

PyTypeObject PyMarkovTrieType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "string_noise.MarkovTrie",
    .tp_doc = "Markov Trie object",
    .tp_basicsize = sizeof(PyMarkovTrieObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = PyMarkovTrie_methods,
    .tp_dealloc = (destructor) PyMarkovTrie_dealloc,
    .tp_new = PyMarkovTrie_new,  // Using the generic allocator
    // ... other type members
};


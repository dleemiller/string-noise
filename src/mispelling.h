#ifndef MISPELLING_H
#define MISPELLING_H

#include <Python.h>

#define ALPHABET_SIZE 100

typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    int isEndOfWord;
    PyObject *misspellings; // Pointer to Python list of misspellings
} TrieNode;

// Declare trieRoot as an extern variable
extern TrieNode *trieRoot;

// Function prototypes
TrieNode* createNode(void);
void insertIntoTrie(TrieNode *root, const char *word, PyObject *misspellings);
void freeTrie(TrieNode *root);
PyObject* build_tree(PyObject *self, PyObject *args);
PyObject* lookup(PyObject *self, PyObject *args);
PyObject* random_mispelling(PyObject *self, PyObject *args, PyObject *kwds);

#endif // MISPELLING_H


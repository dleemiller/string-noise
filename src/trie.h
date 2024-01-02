#ifndef STRING_NOISE_TRIE_H
#define STRING_NOISE_TRIE_H

#include <Python.h>
#include <stdbool.h>

#define TRIE_NODE_SIZE 256

// Trie Node structure
typedef struct TrieNode {
    struct TrieNode *children[TRIE_NODE_SIZE];
    int isEndOfWord;
    PyObject *mapping; // Pointer to Python list of mapping
} TrieNode;

// Function prototypes for trie operations
TrieNode* createNode(void);
void insertIntoTrie(TrieNode *root, const char *word, PyObject *mapping);
void freeTrie(TrieNode *root);
PyObject* build_tree(PyObject *self, PyObject *args);
TrieNode* lookupInTrie(TrieNode *root, const char *word);

// Declaration of the PyTrieObject type
typedef struct {
    PyObject_HEAD
    TrieNode *root;
} PyTrieObject;

// Function prototype for the PyTrie_lookup method
extern PyTypeObject PyTrieType;
PyObject* PyTrie_lookup(PyTrieObject *self, PyObject *args);

typedef struct MarkovNode {
    struct MarkovNode *children[TRIE_NODE_SIZE];
    PyObject *forwardMapping; // Mapping for forward direction
    PyObject *reverseMapping; // Mapping for reverse direction
} MarkovNode;

MarkovNode* createMarkovNode(void);
extern PyTypeObject PyMarkovTrieType;

typedef struct {
    PyObject_HEAD
    MarkovNode *root;
} PyMarkovTrieObject;

void updateCharacterCount(MarkovNode *node, char character, bool isForward);

#endif // STRING_NOISE_TRIE_H


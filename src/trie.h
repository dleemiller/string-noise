#ifndef STRING_NOISE_TRIE_H
#define STRING_NOISE_TRIE_H

#include <Python.h>
#include <stdbool.h>

#define TRIE_NODE_SIZE 256
#define MAX_DEPTH 3

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
// 
// typedef struct MarkovNode {
//     struct MarkovNode *children[TRIE_NODE_SIZE]; // Existing children array
//     int characterCounts[TRIE_NODE_SIZE]; // Array for counting occurrences of Latin-1 characters
// } MarkovNode;
// 
// typedef struct {
//     PyObject_HEAD
//     MarkovNode *forwardRoot; // Root node for forward Trie
//     MarkovNode *reverseRoot; // Root node for reverse Trie
// } PyMarkovTrieObject;
// 
// MarkovNode* createMarkovNode(void);
// extern PyTypeObject PyMarkovTrieType;

#endif // STRING_NOISE_TRIE_H


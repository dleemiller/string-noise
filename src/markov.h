#ifndef MARKOV_H
#define MARKOV_H

#include <Python.h>
#include <stdbool.h>

#define TRIE_NODE_SIZE 256
#define MAX_DEPTH 3

typedef struct MarkovNode {
    struct MarkovNode *children[TRIE_NODE_SIZE]; // Existing children array
    int characterCounts[TRIE_NODE_SIZE]; // Array for counting occurrences of Latin-1 characters
} MarkovNode;

typedef struct {
    PyObject_HEAD
    MarkovNode *forwardRoot; // Root node for forward Trie
    MarkovNode *reverseRoot; // Root node for reverse Trie
} PyMarkovTrieObject;

MarkovNode* createMarkovNode(void);
extern PyTypeObject PyMarkovTrieType;

#endif // MARKOV_H


#ifndef MARKOV_H
#define MARKOV_H

#include <Python.h>
#include <stdbool.h>

#define MTRIE_NODE_SIZE 256
#define MAX_DEPTH 3

#define WHITESPACE_NONE 0
#define WHITESPACE_ZERO 1
#define WHITESPACE_BOUNDARY 2

typedef struct MarkovNode {
    struct MarkovNode *children[MTRIE_NODE_SIZE];
    unsigned int characterCounts[MTRIE_NODE_SIZE];
    int id;
} MarkovNode;

typedef struct {
    PyObject_HEAD
    MarkovNode *forwardRoot;
    MarkovNode *reverseRoot;
    int capacity_full;  // Flag to indicate capacity overflow
    int depth;
} PyMarkovTrieObject;

MarkovNode* createMarkovNode(void);
extern PyTypeObject PyMarkovTrieType;

#endif // MARKOV_H


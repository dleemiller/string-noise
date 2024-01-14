#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "markov.h"

static int lastNodeId = 0;

bool is_printable_latin1(unsigned char c) {
    return (c >= 32 && c <= 126);
}

MarkovNode* createMarkovNode(void) {
    MarkovNode *newNode = malloc(sizeof(MarkovNode));
    if (newNode == NULL) {
        return NULL; // Handle memory allocation failure
    }
    memset(newNode->characterCounts, 0, sizeof(newNode->characterCounts)); // Initialize counts to 0
    for (int i = 0; i < MTRIE_NODE_SIZE; i++) {
        newNode->children[i] = NULL;
    }

    newNode->id = lastNodeId++;
    return newNode;
}

void freeMarkovTrie(MarkovNode *root) {
    if (root == NULL) return;
    for (int i = 0; i < MTRIE_NODE_SIZE; i++) {
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
        printf("Accessing child for character '%c'\n", character);
    }
    if (node->children[(unsigned char)character] == NULL) {
        if (debug) {
            printf("Creating new child node for character %c\n", character);
        }
        node->children[(unsigned char)character] = createMarkovNode();
        if (debug) {
            printf("Assigned ID %d to new child node for character %c\n", node->children[(unsigned char)character]->id, character);
        }
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
void incrementCharacterCount(MarkovNode *node, char character, int *overflowFlag, int debug) {
    unsigned char index = (unsigned char)character;
    if (node->characterCounts[index] == UINT_MAX) {
        *overflowFlag = 1;  // Set the overflow flag
        return;
    }
    if (debug) {
        printf("Increment %d %c\n", index, index);
    }
    node->characterCounts[index]++;
}

// Helper function to check if a char is a multi-byte UTF-8 character
bool isMultiByteChar(char c) {
    // Check if the most significant bit is set (indicative of multi-byte characters in UTF-8)
    return (c & 0x80) != 0;
}

// Helper functions for character validation and processing
static int is_valid_character(char c) {
    // Assuming these functions check if the character is a valid multi-byte character or a printable Latin-1 character
    int valid = !isMultiByteChar(c) || is_printable_latin1(c);
    return valid;
}


static PyObject* index_string_MarkovTrie(PyMarkovTrieObject *self, PyObject *args, PyObject *kwds) {
    PyObject *inputStringObj;
    int debug = 0;

    static char *kwlist[] = {"inputString", "debug", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "U|p", kwlist, &inputStringObj, &debug)) {
        return NULL;
    }

    int depth = self->depth;
    Py_ssize_t length = PyUnicode_GetLength(inputStringObj);

    if (debug) {
        const char *inputString = PyUnicode_AsUTF8(inputStringObj);
        printf("Received string: %s, depth: %d\n", inputString, depth);
        printf("Input string length: %zu\n", length);
    }

    if (depth < 2) {
        PyErr_SetString(PyExc_ValueError, "Depth must be at least 2.");
        return NULL;
    }

    if (self->capacity_full) {
        PyErr_SetString(PyExc_RuntimeError, "Trie capacity is full, cannot index more strings.");
        return NULL;
    }

    if (length < depth) {
        if (debug) {
            printf("String too short for processing. Required length: %d, Actual length: %zu\n", depth, length);
        }
        return PyLong_FromLong(0);
    }

    int sequenceCount = 0;
    int overflowFlag = 0;

    for (Py_ssize_t i = 0; i <= length - depth; i++) {
        if (debug) {
            printf("Processing sequence starting at index %zu\n", i);
        }

        // Forward indexing
        MarkovNode *currentNode = self->forwardRoot;
        int validSequence = 1;
        for (int j = 0; j < depth - 1; j++) {
            PyObject *charObj = PyUnicode_Substring(inputStringObj, i + j, i + j + 1);
            if (charObj == NULL || !PyUnicode_Check(charObj) || PyUnicode_GetLength(charObj) != 1) {
                Py_XDECREF(charObj);
                validSequence = 0;
                break;
            }
            const char *charStr = PyUnicode_AsUTF8(charObj);
            if (!is_valid_character(charStr[0])) {
                Py_DECREF(charObj);
                validSequence = 0;
                break;
            }
            currentNode = getOrCreateChild(currentNode, charStr[0], debug);
            Py_DECREF(charObj);
            if (currentNode == NULL) {
                validSequence = 0;
                break;
            }
        }
        if (validSequence && currentNode != NULL) {
            PyObject *lastCharObj = PyUnicode_Substring(inputStringObj, i + depth - 1, i + depth);
            const char *lastCharStr = PyUnicode_AsUTF8(lastCharObj);
            incrementCharacterCount(currentNode, lastCharStr[0], &overflowFlag, debug);
            Py_DECREF(lastCharObj);
            if (overflowFlag) {
                self->capacity_full = 1;
                PyErr_SetString(PyExc_OverflowError, "Character count overflow occurred.");
                return NULL;
            }
        }

        // Reverse indexing
        currentNode = self->reverseRoot;
        validSequence = 1;
        for (int j = 0; j < depth - 1; j++) {
            PyObject *charObj = PyUnicode_Substring(inputStringObj, length - i - j - 1, length - i - j);
            if (charObj == NULL || !PyUnicode_Check(charObj) || PyUnicode_GetLength(charObj) != 1) {
                Py_XDECREF(charObj);
                validSequence = 0;
                break;
            }
            const char *charStr = PyUnicode_AsUTF8(charObj);
            if (!is_valid_character(charStr[0])) {
                Py_DECREF(charObj);
                validSequence = 0;
                break;
            }

            currentNode = getOrCreateChild(currentNode, charStr[0], debug);
            Py_DECREF(charObj);
            if (currentNode == NULL) {
                validSequence = 0;
                break;
            }
        }
        if (validSequence && currentNode != NULL) {
            PyObject *lastCharObj = PyUnicode_Substring(inputStringObj, i, i + 1);
            const char *lastCharStr = PyUnicode_AsUTF8(lastCharObj);
            incrementCharacterCount(currentNode, lastCharStr[0], &overflowFlag, debug);
            Py_DECREF(lastCharObj);
            if (overflowFlag) {
                self->capacity_full = 1;
                PyErr_SetString(PyExc_OverflowError, "Character count overflow occurred.");
                return NULL;
            }
        }

        sequenceCount += validSequence;
    }

    if (debug) {
        printf("Indexed %d sequences of depth %d\n", sequenceCount, depth);
    }
    return PyLong_FromLong(sequenceCount);
}


void traverseMarkovNode(MarkovNode *node, PyObject *dict, int depth, int debug) {
    if (node == NULL || depth == 0) return;

    for (int i = 0; i < 256; i++) {
        if (!is_printable_latin1((unsigned char)i)) {
            continue; // Skip non-printable Latin-1 characters
        }

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

static PyObject* dump_MarkovTrie(PyMarkovTrieObject *self, PyObject *args, PyObject *kwds) {
    int debug = 0;  // Default value for debug flag

    static char *kwlist[] = {"debug", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|p", kwlist, &debug)) {
        return NULL;
    }

    PyObject *result = PyDict_New();
    PyObject *forwardDict = PyDict_New();
    PyObject *reverseDict = PyDict_New();

    // Traverse for forward and reverse mappings with debug flag
    if (self->forwardRoot != NULL) {
        traverseMarkovNode(self->forwardRoot, forwardDict, self->depth, 0);
    }
    if (self->reverseRoot != NULL) {
        traverseMarkovNode(self->reverseRoot, reverseDict, self->depth, debug);
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

bool loadTrieFromDict(MarkovNode *node, PyObject *dict, int *capacityFullFlag) {
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
            if (!node->children[(unsigned char)character] || !loadTrieFromDict(node->children[(unsigned char)character], value, capacityFullFlag)) {
                return false;
            }
        }
        // If value is an integer, set the character count
        else if (PyLong_Check(value)) {
            unsigned long count = PyLong_AsUnsignedLong(value);
            if (count > UINT_MAX) {
                PyErr_SetString(PyExc_OverflowError, "Character count exceeds maximum capacity");
                *capacityFullFlag = 1;
                return false;
            }
            node->characterCounts[(unsigned char)character] = (unsigned int)count;
            if (count == UINT_MAX) {
                *capacityFullFlag = 1;
            }
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

    // Clear existing Trie and reset capacity_full flag
    freeMarkovTrie(self->forwardRoot);
    self->forwardRoot = createMarkovNode();
    freeMarkovTrie(self->reverseRoot);
    self->reverseRoot = createMarkovNode();
    self->capacity_full = 0;

    int capacityFullFlag = 0;

    loadTrieFromDict(self->forwardRoot, forwardDict, &capacityFullFlag);
    loadTrieFromDict(self->reverseRoot, reverseDict, &capacityFullFlag);
    if (capacityFullFlag) {
        self->capacity_full = 1;
    }

    Py_RETURN_NONE;
}

int is_whitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

int is_multi_byte_unicode(const char *character) {
    if (!character) {
        return 0;
    }
    return (unsigned char)character[0] >= 0x80;
}


PyObject *construct_ngram(PyObject *inputString, Py_ssize_t index, const char *direction, int depth) {
    PyObject *ngram = PyList_New(0);
    Py_ssize_t length = PyUnicode_GetLength(inputString);

    if (strcmp(direction, "forward") == 0 && index >= depth - 1) {
        for (Py_ssize_t i = index - depth + 1; i <= index; ++i) {
            PyObject *charObj = PyUnicode_Substring(inputString, i, i + 1);
            const char *charStr = PyUnicode_AsUTF8(charObj);
            if (is_multi_byte_unicode(charStr)) {
                Py_DECREF(charObj);
                Py_DECREF(ngram);
                return PyList_New(0); // Return empty ngram if multi-byte character found
            }
            PyList_Append(ngram, charObj);
            Py_DECREF(charObj);
        }
    } else if (strcmp(direction, "reverse") == 0 && index <= length - depth) {
        for (Py_ssize_t i = index; i < index + depth; ++i) {
            PyObject *charObj = PyUnicode_Substring(inputString, i, i + 1);
            const char *charStr = PyUnicode_AsUTF8(charObj);
            if (is_multi_byte_unicode(charStr)) {
                Py_DECREF(charObj);
                Py_DECREF(ngram);
                return PyList_New(0); // Return empty ngram if multi-byte character found
            }
            PyList_Append(ngram, charObj);
            Py_DECREF(charObj);
        }
    }

    return ngram;
}


PyObject *calculate_character_counts(MarkovNode *node, PyObject *ngram, int depth, const char *direction, int debug) {
    if (debug) {
        printf("Calculating character counts for ngram in %s direction\n", direction);
    }

    PyObject *counts = PyDict_New();
    Py_ssize_t ngramSize = PyList_Size(ngram);

    if (ngramSize != depth) {
        if (debug) {
            printf("Incomplete ngram, returning zero counts\n");
        }
        return counts;
    }

    MarkovNode *currentNode = node;
    if (strcmp(direction, "forward") == 0) {
        for (int i = 0; i < depth - 1; ++i) {
            PyObject *charObj = PyList_GetItem(ngram, i);
            const char *charStr = PyUnicode_AsUTF8(charObj);
            if (!charStr) {
                if (debug) {
                    printf("Error converting Python string object to C string at index %d\n", i);
                }
                Py_DECREF(counts);
                return NULL;
            }
            
            unsigned char index = (unsigned char)charStr[0];
            currentNode = currentNode->children[index];
            if (currentNode == NULL) {
                if (debug) {
                    printf("Child node not found for '%c' at depth %d\n", charStr[0], i + 1);
                }
                return counts; // Return zero counts if the path does not exist
            }
        }
    } else if (strcmp(direction, "reverse") == 0) {
        for (int i = depth - 1; i > 0; --i) {
            PyObject *charObj = PyList_GetItem(ngram, i);
            const char *charStr = PyUnicode_AsUTF8(charObj);
            if (!charStr) {
                if (debug) {
                    printf("Error converting Python string object to C string at index %d\n", i);
                }
                Py_DECREF(counts);
                return NULL;
            }

            unsigned char index = (unsigned char)charStr[0];
            currentNode = currentNode->children[index];
            if (currentNode == NULL) {
                if (debug) {
                    printf("Child node not found for '%c' at depth %d\n", charStr[0], i);
                }
                return counts; // Return zero counts if the path does not exist
            }
        }
    }

    if (debug) {
        printf("Counting characters at the last node of the ngram\n");
    }

    // currentNode now points to the last node of the ngram
    for (int i = 0; i < 256; ++i) {
        if (!is_printable_latin1((unsigned char)i)) {
            continue; // Skip non-printable Latin-1 characters
        }
        unsigned int count = currentNode->characterCounts[i];
        if (count > 0) {
            PyObject *charObj = PyUnicode_FromOrdinal((unsigned char)i);
            PyObject *countObj = PyLong_FromLong(count);
            if (!charObj || !countObj) {
                if (debug) {
                    printf("Error creating Python objects for character '%c'\n", (char)i);
                }
                Py_XDECREF(charObj);
                Py_XDECREF(countObj);
                Py_DECREF(counts);
                return NULL;
            }

            PyDict_SetItem(counts, charObj, countObj);
            if (debug) {
                printf("Added count %u for character '%c'\n", count, (char)i);
            }
            Py_DECREF(charObj);
            Py_DECREF(countObj);
        }
    }

    if (debug) {
        printf("Finished counting characters\n");
    }

    return counts;
}


PyObject *normalize_counts_to_probabilities(PyObject *counts) {
    PyObject *probabilities = PyDict_New();
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    long total = 0;

    // Calculate total count
    while (PyDict_Next(counts, &pos, &key, &value)) {
        total += PyLong_AsLong(value);
    }

    if (total == 0) {
        return probabilities;
    }

    // Normalize counts
    pos = 0;
    while (PyDict_Next(counts, &pos, &key, &value)) {
        double probability = (double)PyLong_AsLong(value) / total;
        PyObject *probObj = PyFloat_FromDouble(probability);
        PyDict_SetItem(probabilities, key, probObj);
        Py_DECREF(probObj);
    }

    return probabilities;
}

PyObject *normalize_probabilities(PyObject *probabilities) {
    PyObject *normalizedProbabilities = PyDict_New();
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    double total = 0.0;

    // Calculate total probability
    while (PyDict_Next(probabilities, &pos, &key, &value)) {
        total += PyFloat_AsDouble(value);
    }

    if (total == 0.0) {
        return normalizedProbabilities;
    }

    // Normalize probabilities
    pos = 0;
    while (PyDict_Next(probabilities, &pos, &key, &value)) {
        double normalizedProbability = PyFloat_AsDouble(value) / total;
        PyObject *normalizedProbObj = PyFloat_FromDouble(normalizedProbability);
        PyDict_SetItem(normalizedProbabilities, key, normalizedProbObj);
        Py_DECREF(normalizedProbObj);
    }

    return normalizedProbabilities;
}


char randomly_select_character(PyObject *probabilities) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    double randValue = (double)rand() / RAND_MAX;
    double cumulative = 0.0;

    while (PyDict_Next(probabilities, &pos, &key, &value)) {
        cumulative += PyFloat_AsDouble(value);
        if (randValue <= cumulative) {
            const char *charStr = PyUnicode_AsUTF8(key);
            return charStr[0];
        }
    }

    // In case no character is selected (should not happen if probabilities sum to 1)
    return '\0';
}


void zeroOutWhitespaceCounts(PyObject *characterCounts) {
    unsigned char c = 0;
    do {
        if (is_whitespace(c)) {
            PyObject *whitespaceChar = PyUnicode_FromFormat("%c", c);
            PyDict_SetItem(characterCounts, whitespaceChar, PyLong_FromLong(0));
            Py_DECREF(whitespaceChar);
        }
        c++;
    } while (c != 0);
}


static PyObject* replace_MarkovTrie(PyMarkovTrieObject *self, PyObject *args, PyObject *kwds) {
    PyObject *inputString;
    double probability = 0.5;
    double reverse_weight = 1.0;
    Py_ssize_t stride = 1;
    int debug = 0, seed = -1;
    int zero_whitespace = WHITESPACE_NONE; // Default to boundary
    static char *kwlist[] = {"inputString", "probability", "reverse_weight", "stride", "debug", "zero_whitespace", "seed", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "U|ddnpbi", kwlist, &inputString, &probability, &reverse_weight, &stride, &debug, &zero_whitespace, &seed)) {
        return NULL;
    }

    if (zero_whitespace != WHITESPACE_NONE && zero_whitespace != WHITESPACE_ZERO && zero_whitespace != WHITESPACE_BOUNDARY) {
        PyErr_SetString(PyExc_ValueError, "Invalid option for zero_whitespace");
        return NULL;
    }

    if (probability < 0.0 || probability > 1.0) {
        PyErr_SetString(PyExc_ValueError, "Probability must be between 0.0 and 1.0");
        return NULL;
    }

    if (reverse_weight < 0.0) {
        PyErr_SetString(PyExc_ValueError, "Reverse weight must be >= 0");
        return NULL;
    }

    // Ensure stride is greater than or equal to 1
    if (stride < 1) {
        PyErr_SetString(PyExc_ValueError, "stride must be greater than or equal to 1");
        return NULL;
    }

    // Seed the random number generator
    if (seed == -1) {
        srand((unsigned int)clock());
    } else {
        srand((unsigned int)seed);
    }

    if (debug) {
        printf("Debug: Replacing string with probability: %f\n", probability);
    }

    Py_ssize_t length = PyUnicode_GetLength(inputString);
    PyObject *resultString = PyUnicode_FromString("");
    Py_ssize_t skip = 0;
    for (Py_ssize_t i = 0; i < length; i++) {
        PyObject *currentChar = PyUnicode_Substring(inputString, i, i + 1);
        const char *currentCharStr = PyUnicode_AsUTF8(currentChar);
        if (skip > 0) {
            skip--;
            PyUnicode_AppendAndDel(&resultString, currentChar);
            continue;
        }

        if (((double)rand() / RAND_MAX) > probability) {
            PyUnicode_AppendAndDel(&resultString, currentChar);
            continue;
        } else if (stride > 1) {
            skip = stride - 1;
        }


        // Skip if character is whitespace or multi-byte
        if (is_whitespace(*currentCharStr) || is_multi_byte_unicode(currentCharStr)) {
            PyUnicode_AppendAndDel(&resultString, currentChar);
            continue;
        }

        if (debug) {
            printf("Debug: Replacing character '%s' at position %zd\n", currentCharStr, i);
        }

        // Construct and validate ngrams
        PyObject *forwardNgram = construct_ngram(resultString, i, "forward", self->depth);
        PyObject *reverseNgram = construct_ngram(inputString, i, "reverse", self->depth);
        
        if (debug) {
            printf("Debug: Forward ngram at position %zd: %s\n", i, PyUnicode_AsUTF8(PyObject_Str(forwardNgram)));
            printf("Debug: Reverse ngram at position %zd: %s\n", i, PyUnicode_AsUTF8(PyObject_Str(reverseNgram)));
        }

        PyObject *forwardCounts = calculate_character_counts(self->forwardRoot, forwardNgram, self->depth, "forward", debug);
        if (!forwardCounts) {
            if (debug) {
                printf("Failed to calculate character counts for forward ngram\n");
            }
            Py_DECREF(forwardNgram);
            Py_DECREF(reverseNgram);
            continue;
        }
        
        PyObject *reverseCounts = calculate_character_counts(self->reverseRoot, reverseNgram, self->depth, "reverse", debug);
        if (!reverseCounts) {
            if (debug) {
                printf("Failed to calculate character counts for reverse ngram\n");
            }
            Py_DECREF(forwardNgram);
            Py_DECREF(reverseNgram);
            Py_DECREF(forwardCounts);
            continue;
        }
        

        // Process zero_whitespace option
        switch (zero_whitespace) {
            case WHITESPACE_ZERO:
                // Zero out counts for all whitespace characters in characterCounts
                zeroOutWhitespaceCounts(forwardCounts);
                zeroOutWhitespaceCounts(reverseCounts);
                break;
            case WHITESPACE_BOUNDARY:
                if (!is_whitespace(*currentCharStr)) {
                    // Check if current character is at a word boundary
                    int atBoundary = 0;
                    PyObject *prevChar = NULL;
                    PyObject *nextChar = NULL;
                    if (i > 0) {
                        prevChar = PyUnicode_Substring(inputString, i - 1, i);
                    }
                    if (i < length - 1) {
                        nextChar = PyUnicode_Substring(inputString, i + 1, i + 2);
                    }

                    if (debug) {
                        printf("Debug: Current character '%s' at index %zd\n", currentCharStr, i);
                        if (prevChar) printf("Debug: Previous character '%s'\n", PyUnicode_AsUTF8(prevChar));
                        if (nextChar) printf("Debug: Next character '%s'\n", PyUnicode_AsUTF8(nextChar));
                    }

                    if (i == 0 || i == length - 1) {
                        atBoundary = 1; // At the start or end of the string
                    } else if ((prevChar && is_whitespace(*PyUnicode_AsUTF8(prevChar))) || 
                               (nextChar && is_whitespace(*PyUnicode_AsUTF8(nextChar)))) {
                        atBoundary = 1; // Adjacent to a whitespace character
                    }

                    if (debug && !atBoundary) {
                        printf("Debug: Character '%s' at index %zd is not at a boundary.\n", currentCharStr, i);
                    }

                    if (!atBoundary) {
                        // Zero out counts for all whitespace characters in characterCounts
                        zeroOutWhitespaceCounts(forwardCounts);
                        zeroOutWhitespaceCounts(reverseCounts);
                    }

                    Py_XDECREF(prevChar);
                    Py_XDECREF(nextChar);
                }
                break;
            default:
                // WHITESPACE_NONE: Do nothing
                break;
        }

        // Calculate the sum of all counts in forwardCounts and reverseCounts
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        unsigned long sumOfCounts = 0;
        
        // Calculate sum for forward counts
        pos = 0;
        while (PyDict_Next(forwardCounts, &pos, &key, &value)) {
            sumOfCounts += PyLong_AsUnsignedLong(value);
        }
        
        // Calculate sum for reverse counts
        pos = 0;
        while (PyDict_Next(reverseCounts, &pos, &key, &value)) {
            sumOfCounts += PyLong_AsUnsignedLong(value);
        }
        
        // Check if the combined sum of counts is zero
        if (sumOfCounts == 0) {
            PyUnicode_AppendAndDel(&resultString, currentChar);
            continue; // No valid counts, append original character and continue
        }

        PyObject *normalizedForward = normalize_counts_to_probabilities(forwardCounts);
        PyObject *normalizedReverse = normalize_counts_to_probabilities(reverseCounts);
        
        // Combine the probabilities from forward and reverse ngrams
        PyObject *combinedProbabilities = PyDict_Copy(normalizedForward); // Create a copy of the normalized forward probabilities
        
        if (combinedProbabilities == NULL) {
            // Handle error: Failed to copy the dictionary
            Py_DECREF(normalizedForward);
            Py_DECREF(normalizedReverse);
            return NULL;
        }

        // Add probabilities from reverse ngram, combining with existing ones if necessary
        pos = 0;
        while (PyDict_Next(normalizedReverse, &pos, &key, &value)) {
            PyObject *existingValue = PyDict_GetItem(combinedProbabilities, key);
            if (existingValue) {
                double combinedProbability = reverse_weight * PyFloat_AsDouble(value) + PyFloat_AsDouble(existingValue);
                PyObject *newProb = PyFloat_FromDouble(combinedProbability);
                PyDict_SetItem(combinedProbabilities, key, newProb);
                Py_DECREF(newProb);
            } else {
                PyDict_SetItem(combinedProbabilities, key, value);
            }
        }


        // Normalize counts to probabilities and select replacement character
        PyObject *normalizedProbabilities = normalize_probabilities(combinedProbabilities);
        
        if (debug) {
            PyObject *probStr = PyObject_Str(normalizedProbabilities);
            printf("Debug: Normalized probabilities: %s\n", PyUnicode_AsUTF8(probStr));
            Py_DECREF(probStr);
        }

        char replacementChar = randomly_select_character(normalizedProbabilities);
        
        if (debug) {
            printf("Debug: Replacement character: %c\n", replacementChar);
        }

        PyObject *replacementCharStr = PyUnicode_FromFormat("%c", replacementChar);

        // Replace character at position i
        PyUnicode_AppendAndDel(&resultString, replacementCharStr);

        // Cleanup
        Py_DECREF(currentChar);
        Py_DECREF(forwardNgram);
        Py_DECREF(reverseNgram);
        Py_DECREF(forwardCounts);
        Py_DECREF(reverseCounts);
        Py_DECREF(normalizedForward);
        Py_DECREF(normalizedReverse);
        Py_DECREF(combinedProbabilities);
    }

    if (debug) {
        printf("Debug: Result string: %s\n", PyUnicode_AsUTF8(resultString));
    }

    return resultString;
}


static PyMethodDef PyMarkovTrie_methods[] = {
    {"index_string", (PyCFunction)index_string_MarkovTrie, METH_VARARGS | METH_KEYWORDS, "Index a string in the Markov Trie."},
    {"dump", (PyCFunction)dump_MarkovTrie, METH_VARARGS | METH_KEYWORDS, "Dump the Markov Trie into a Python dictionary."},
    {"load", (PyCFunction)load_MarkovTrie, METH_VARARGS, "Load data into the Markov Trie from a dictionary."},
    {"replace", (PyCFunction)replace_MarkovTrie, METH_VARARGS | METH_KEYWORDS, "Replace characters in the string based on Markov probabilities."},
    {NULL} // Sentinel
};

static PyObject *PyMarkovTrie_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"depth", NULL};
    int depth = 3; // Default depth if not specified

    PyMarkovTrieObject *self;
    self = (PyMarkovTrieObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        // Parse keyword arguments for depth
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &depth)) {
            Py_DECREF(self);
            return NULL;
        }

        if (depth < 2) {
            PyErr_SetString(PyExc_ValueError, "Depth must be at least 2.");
            Py_DECREF(self);
            return NULL;
        }
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

        self->depth = depth;
    }
    return (PyObject *)self;
}

static PyObject *PyMarkovTrie_get_depth(PyMarkovTrieObject *self, void *closure) {
    return PyLong_FromLong(self->depth);
}

static PyGetSetDef PyMarkovTrie_getsetters[] = {
    {"depth", (getter)PyMarkovTrie_get_depth, NULL, "depth of the trie", NULL},
    {NULL}  // Sentinel
};


PyTypeObject PyMarkovTrieType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "string_noise.MarkovTrie",
    .tp_doc = "Markov Trie object",
    .tp_basicsize = sizeof(PyMarkovTrieObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = PyMarkovTrie_methods,
    .tp_dealloc = (destructor) PyMarkovTrie_dealloc,
    .tp_new = PyMarkovTrie_new,
    .tp_getset = PyMarkovTrie_getsetters,  // Add the getsetters here
};


#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "markov.h"

bool is_printable_latin1(char c) {
    unsigned char uc = (unsigned char)c;
    return (uc >= 32 && uc <= 126) || (uc >= 160 && uc <= 255);
}

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
void incrementCharacterCount(MarkovNode *node, char character, int *overflowFlag) {
    unsigned char index = (unsigned char)character;
    if (node->characterCounts[index] == UINT_MAX) {
        *overflowFlag = 1;  // Set the overflow flag
        return;
    }
    node->characterCounts[index]++;
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

    if (self->capacity_full) {
        PyErr_SetString(PyExc_RuntimeError, "Trie capacity is full, cannot index more strings.");
        return NULL;
    }

    int overflowFlag = 0;

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

        if (!is_printable_latin1(inputString[i]) || 
            !is_printable_latin1(inputString[i + 1]) || 
            !is_printable_latin1(inputString[i + 2])) {
            continue; // Skip non-printable Latin-1 characters
        }

        if (debug) {
            printf("Processing forward trigram: %c%c%c\n", inputString[i], inputString[i + 1], inputString[i + 2]);
        }

        // Forward indexing
        MarkovNode *firstNode = getOrCreateChild(self->forwardRoot, inputString[i], debug);
        if (firstNode == NULL) return NULL;
        MarkovNode *secondNode = getOrCreateChild(firstNode, inputString[i + 1], debug);
        if (secondNode == NULL) return NULL;
        incrementCharacterCount(secondNode, inputString[i + 2], &overflowFlag);
        if (overflowFlag) {
            self->capacity_full = 1;  // Set the capacity_full flag of the trie
            PyErr_SetString(PyExc_OverflowError, "Character count overflow occurred.");
            return NULL;
        }


        // Process the reverse trigram starting from the end of the string
        if (debug) {
            printf("Processing reverse trigram: %c%c%c\n", inputString[length - i - 1], inputString[length - i - 2], inputString[length - i - 3]);
        }

        // Reverse indexing
        MarkovNode *lastNode = getOrCreateChild(self->reverseRoot, inputString[length - i - 1], debug);
        if (lastNode == NULL) return NULL;
        MarkovNode *secondLastNode = getOrCreateChild(lastNode, inputString[length - i - 2], debug);
        if (secondLastNode == NULL) return NULL;
        incrementCharacterCount(secondLastNode, inputString[length - i - 3], &overflowFlag);
        if (overflowFlag) {
            self->capacity_full = 1;  // Set the capacity_full flag of the trie
            PyErr_SetString(PyExc_OverflowError, "Character count overflow occurred.");
            return NULL;
        }

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
        if (!is_printable_latin1((char)i)) {
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


PyObject *construct_trigram(PyObject *inputString, Py_ssize_t index, const char *direction) {
    PyObject *trigram = PyList_New(0);
    Py_ssize_t length = PyUnicode_GetLength(inputString);

    if (strcmp(direction, "forward") == 0 && index >= 2) {
        for (Py_ssize_t i = index - 2; i <= index; ++i) {
            PyObject *charObj = PyUnicode_Substring(inputString, i, i + 1);
            const char *charStr = PyUnicode_AsUTF8(charObj);
            if (is_multi_byte_unicode(charStr)) {
                Py_DECREF(charObj);
                Py_DECREF(trigram);
                return PyList_New(0); // Return empty trigram if multi-byte character found
            }
            PyList_Append(trigram, charObj);
            Py_DECREF(charObj);
        }
    } else if (strcmp(direction, "reverse") == 0 && index <= length - 3) {
        for (Py_ssize_t i = index; i <= index + 2; ++i) {
            PyObject *charObj = PyUnicode_Substring(inputString, i, i + 1);
            const char *charStr = PyUnicode_AsUTF8(charObj);
            if (is_multi_byte_unicode(charStr)) {
                Py_DECREF(charObj);
                Py_DECREF(trigram);
                return PyList_New(0); // Return empty trigram if multi-byte character found
            }
            PyList_Append(trigram, charObj);
            Py_DECREF(charObj);
        }
    }

    return trigram;
}


PyObject *calculate_character_counts(MarkovNode *node, PyObject *trigram, const char *direction, int debug) {
    if (debug) {
        printf("Calculating character counts for trigram in %s direction\n", direction);
    }

    PyObject *counts = PyDict_New();
    Py_ssize_t trigramSize = PyList_Size(trigram);

    if (trigramSize < 3) {
        if (debug) {
            printf("Incomplete trigram, returning zero counts\n");
        }
        return counts;
    }

    // Determine indices based on direction
    int firstIndexPosition = (strcmp(direction, "forward") == 0) ? 0 : 2;
    int secondIndexPosition = 1;

    // Extract the first two characters of the trigram for indexing
    PyObject *firstCharObj = PyList_GetItem(trigram, firstIndexPosition);
    PyObject *secondCharObj = PyList_GetItem(trigram, secondIndexPosition);

    const char *firstCharStr = PyUnicode_AsUTF8(firstCharObj);
    const char *secondCharStr = PyUnicode_AsUTF8(secondCharObj);

    if (!firstCharStr || !secondCharStr) {
        if (debug) {
            printf("Error converting Python string objects to C strings\n");
        }
        Py_DECREF(counts);
        return NULL;
    }

    if (debug) {
        printf("Trigram characters: '%c', '%c'\n", firstCharStr[0], secondCharStr[0]);
    }

    unsigned char firstIndex = (unsigned char)firstCharStr[0];
    unsigned char secondIndex = (unsigned char)secondCharStr[0];

    MarkovNode *firstChild = node->children[firstIndex];
    if (firstChild == NULL) {
        if (debug) {
            printf("First child node not found for '%c'\n", firstCharStr[0]);
        }
        return counts; // Return zero counts if the path does not exist
    }

    MarkovNode *secondChild = firstChild->children[secondIndex];
    if (secondChild == NULL) {
        if (debug) {
            printf("Second child node not found for '%c'\n", secondCharStr[0]);
        }
        return counts; // Return zero counts if the path does not exist
    }

    if (debug) {
        printf("Counting characters at second child node\n");
    }

    // At this point, secondChild contains the counts we're interested in
    for (int i = 0; i < 256; ++i) {
        if (!is_printable_latin1((char)i)) {
            continue; // Skip non-printable Latin-1 characters
        }
        unsigned int count = secondChild->characterCounts[i];
        if (count > 0) {
            //PyObject *charObj = PyUnicode_FromFormat("%c", (char)i);
            PyObject *charObj = PyUnicode_FromOrdinal((unsigned char)i);
            PyObject *countObj = PyLong_FromLong(count);

            if (!charObj || !countObj) {
                if (debug) {
                    printf("Error creating Python objects for character '%c' %d\n", (char)i, i);
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
        } else if (stride > 1) {
            skip = stride - 1;
        }

        if (((double)rand() / RAND_MAX) > probability) {
            PyUnicode_AppendAndDel(&resultString, currentChar);
            continue;
        }

        // Skip if character is whitespace or multi-byte
        if (is_whitespace(*currentCharStr) || is_multi_byte_unicode(currentCharStr)) {
            PyUnicode_AppendAndDel(&resultString, currentChar);
            continue;
        }

        if (debug) {
            printf("Debug: Replacing character '%s' at position %zd\n", currentCharStr, i);
        }

        // Construct and validate trigrams
        PyObject *forwardTrigram = construct_trigram(resultString, i, "forward");
        PyObject *reverseTrigram = construct_trigram(inputString, i, "reverse");
        
        if (debug) {
            printf("Debug: Forward trigram at position %zd: %s\n", i, PyUnicode_AsUTF8(PyObject_Str(forwardTrigram)));
            printf("Debug: Reverse trigram at position %zd: %s\n", i, PyUnicode_AsUTF8(PyObject_Str(reverseTrigram)));
        }

        PyObject *forwardCounts = calculate_character_counts(self->forwardRoot, forwardTrigram, "forward", debug);
        if (!forwardCounts) {
            if (debug) {
                printf("Failed to calculate character counts for forward trigram\n");
            }
            Py_DECREF(forwardTrigram);
            Py_DECREF(reverseTrigram);
            continue;
        }
        
        PyObject *reverseCounts = calculate_character_counts(self->reverseRoot, reverseTrigram, "reverse", debug);
        if (!reverseCounts) {
            if (debug) {
                printf("Failed to calculate character counts for reverse trigram\n");
            }
            Py_DECREF(forwardTrigram);
            Py_DECREF(reverseTrigram);
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
        
        // Combine the probabilities from forward and reverse trigrams
        PyObject *combinedProbabilities = PyDict_Copy(normalizedForward); // Create a copy of the normalized forward probabilities
        
        if (combinedProbabilities == NULL) {
            // Handle error: Failed to copy the dictionary
            Py_DECREF(normalizedForward);
            Py_DECREF(normalizedReverse);
            return NULL;
        }

        // Add probabilities from reverse trigram, combining with existing ones if necessary
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
        Py_DECREF(forwardTrigram);
        Py_DECREF(reverseTrigram);
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
    {"index_string", (PyCFunction)index_string_MarkovTrie, METH_VARARGS, "Index a string in the Markov Trie."},
    {"dump", (PyCFunction)dump_MarkovTrie, METH_VARARGS, "Dump the Markov Trie into a Python dictionary."},
    {"load", (PyCFunction)load_MarkovTrie, METH_VARARGS, "Load data into the Markov Trie from a dictionary."},
    {"replace", (PyCFunction)replace_MarkovTrie, METH_VARARGS | METH_KEYWORDS, "Replace characters in the string based on Markov probabilities."},
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
    .tp_new = PyMarkovTrie_new,
};


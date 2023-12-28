#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "random.h"

// Define default control characters
#define DEFAULT_VOWEL_MASK 0x06     // Example: ACK for vowels
#define DEFAULT_CONSONANT_MASK 0x07 // Example: BEL for consonants
#define DEFAULT_NWS_MASK 0x08       // Example: BS for non-whitespace/other
#define DEFAULT_GENERAL_MASK 0x09   // Example: HT for general mask

int is_vowel(char c) {
    // Assuming English vowels
    char lower_c = tolower(c);
    return (lower_c == 'a' || lower_c == 'e' || lower_c == 'i' || lower_c == 'o' || lower_c == 'u');
}

int is_consonant(char c) {
    char lower_c = tolower(c);
    return isalpha(lower_c) && !is_vowel(lower_c);
}


// Main function for random masking with debug flag
PyObject* random_masking(PyObject *self, PyObject *args, PyObject *keywds) {
    char *original_string;
    double probability = 0.1;
    int min_consecutive = 1;
    int max_consecutive = 2;
    char vowel_mask = DEFAULT_VOWEL_MASK;
    char consonant_mask = DEFAULT_CONSONANT_MASK;
    char nws_mask = DEFAULT_NWS_MASK;
    char general_mask = DEFAULT_GENERAL_MASK;
    double general_mask_probability = 0.5;
    long seed = -1;
    int debug = 0;
    static char *kwlist[] = {"original_string", "probability", "min_consecutive", "max_consecutive",
                             "vowel_mask", "consonant_mask", "nws_mask", "general_mask", "general_mask_probability", "seed", "debug", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|diiccccdli", kwlist, &original_string,
                                     &probability, &min_consecutive, &max_consecutive, 
                                     &vowel_mask, &consonant_mask, &nws_mask, &general_mask, &general_mask_probability, &seed, &debug)) {
        return NULL;
    }

    // Seed the random number generator
    if (seed == -1) {
        srand((unsigned int)time(NULL));
    } else {
        srand((unsigned int)seed);
    }

    size_t original_len = strlen(original_string);
    char *new_string = malloc(original_len + 1);
    if (!new_string) {
        PyErr_NoMemory();
        return NULL;
    }

    if (debug) {
        printf("Debug: Original string: %s\n", original_string);
    }

    size_t new_index = 0;
    for (size_t i = 0; i < original_len;) {
        // Check if the current character is whitespace and handle it
        if (isspace((unsigned char)original_string[i])) {
            new_string[new_index++] = original_string[i++];
            continue;
        }
    
        // Apply masking logic to non-whitespace characters
        if ((double)rand() / RAND_MAX < probability) {
            int chars_in = min_consecutive + (rand() % (max_consecutive - min_consecutive + 1));
            chars_in = process_chars_in(original_string, i, chars_in);
    
            if (debug) {
                printf("Debug: Masking %d characters starting at index %zu\n", chars_in, i);
            }
    
            for (int j = 0; j < chars_in && i + j < original_len; ++j) {
                char mask_char = ((double)rand() / RAND_MAX < general_mask_probability) ? general_mask : nws_mask;
    
                if (mask_char != general_mask) {
                    if (is_vowel(original_string[i + j])) {
                        mask_char = vowel_mask;
                    } else if (is_consonant(original_string[i + j])) {
                        mask_char = consonant_mask;
                    } else {
                        mask_char = nws_mask;
                    }
                }
    
                if (debug) {
                    printf("Debug: Applying mask '%c' at index %zu\n", mask_char, new_index);
                }
    
                new_string[new_index++] = mask_char;
            }
    
            i += chars_in;
        } else {
            new_string[new_index++] = original_string[i++];
        }
    }
    new_string[new_index] = '\0';

    if (debug) {
        printf("Debug: Final masked string: %s\n", new_string);
    }

    PyObject *result = Py_BuildValue("s", new_string);
    free(new_string);
    return result;
}


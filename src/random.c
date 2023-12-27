#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


int process_chars_in(char *original_string, size_t start, int chars_in) {
    for (int j = 0; j < chars_in; ++j) {
        if (isspace((unsigned char)original_string[start + j])) {
            return j; // Return the new length up to the whitespace character
        }
    }
    return chars_in; // Return the original length if no whitespace is encountered
}


PyObject* random_replacement(PyObject *self, PyObject *args, PyObject *keywds) {
    char *original_string;
    char *charset;
    int min_chars_in = 1;
    int max_chars_in = 2;
    int min_chars_out = 1;
    int max_chars_out = 2;
    double probability = 0.1;
    long seed = -1;
    int debug = 0;  // Default value for debug is false

    static char *kwlist[] = {"original_string", "charset", "min_chars_in", "max_chars_in", 
                             "min_chars_out", "max_chars_out", "probability", "seed", "debug", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|iiiidli", kwlist, &original_string, &charset,
                                     &min_chars_in, &max_chars_in, &min_chars_out, &max_chars_out, 
                                     &probability, &seed, &debug)) {
        return NULL;
    }

    // Validate min and max chars in
    if (min_chars_in < 0 || max_chars_in < min_chars_in) {
        PyErr_SetString(PyExc_ValueError, "Invalid range for min_chars_in and max_chars_in.");
        return NULL;
    }

    // Seed the random number generator
    if (seed == -1) {
        srand((unsigned int)time(NULL));
    } else {
        srand((unsigned int)seed);
    }

    size_t original_len = strlen(original_string);
    size_t charset_len = strlen(charset);
    if (charset_len == 0) {
        PyErr_SetString(PyExc_ValueError, "Charset cannot be empty.");
        return NULL;
    }

    char *new_string = malloc(original_len * max_chars_out + 1);
    if (!new_string) {
        PyErr_NoMemory();
        return NULL;
    }

    if (debug) {
        printf("Debug info: \n");
        printf("Original string: %s\n", original_string);
        printf("Charset: %s\n", charset);
        // Additional debug information can be printed here
    }

    size_t new_index = 0;
    for (size_t i = 0; i < original_len;) {
        if (debug) {
            printf("Processing character %c at index %zu\n", original_string[i], i);
        }
        char current_char = original_string[i];

        // Check if the current character is a whitespace
        if (isspace((unsigned char)current_char)) {
            new_string[new_index++] = current_char;
            i++;
            continue;
        }
    
        if ((double)rand() / RAND_MAX > probability) {
            new_string[new_index++] = original_string[i++];
            continue;
        }
    
        int chars_in = min_chars_in + (rand() % (max_chars_in - min_chars_in + 1));
        int chars_out = min_chars_out + (rand() % (max_chars_out - min_chars_out + 1));

        // Recursively adjust chars_in if whitespace is encountered
        chars_in = process_chars_in(original_string, i, chars_in);

        if (chars_in == 0) {
            new_string[new_index++] = original_string[i];
        }

        for (int j = 0; j < chars_out; ++j) {
            new_string[new_index++] = charset[rand() % charset_len];
        }

        if (debug) {
            printf("chars_in: %d, chars_out: %d\n", chars_in, chars_out);
        }
    
        i += (chars_in > 0 ? chars_in : 1);
    
        if (debug) {
            printf("New string so far: %.*s\n", (int)new_index, new_string);
        }
    }

    new_string[new_index] = '\0';

    if (debug) {
        printf("Final new string: %s\n", new_string);
    }

    PyObject *result = Py_BuildValue("s", new_string);
    free(new_string);

    return result;
}



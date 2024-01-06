#include <Python.h>
#include <string.h>

#include "constants.h"


PyObject* tokenize(PyObject* self, PyObject* args, PyObject *keywds) {
    const char* text;
    int max_length = 16; // Default value for max_length
    int pad_id = DEFAULT_PAD; // Default value for pad_id
    static char *kwlist[] = {"text", "max_length", "pad_id", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|ii", kwlist, &text, &max_length, &pad_id)) {
        return NULL;
    }

    PyObject* list = PyList_New(0);
    if (!list) return NULL;

    const char* word_start = text;
    const char* text_end = text + strlen(text);
    for (const char* it = text; it <= text_end; ++it) {
        // Check for word end (whitespace or end of string)
        if (isspace(*it) || *it == '\0') {
            int word_length = it - word_start + ((*it != '\0') ? 1 : 0); // Include non-space whitespace
            int bytes_to_copy = (word_length > max_length) ? max_length : word_length;

            unsigned char buffer[max_length];
            memcpy(buffer, word_start, bytes_to_copy);

            // Adjust for 'END_SPACE' and 'END' tokens
            if (bytes_to_copy < max_length) {
                if (*it == ' ' && (it != text_end)) {
                    // Add 'END_SPACE' if followed by a space and not at the end
                    buffer[bytes_to_copy++] = (unsigned char)DEFAULT_END_SPACE;
                } else if (isspace(*it) || *it == '\0') {
                    // Add 'END' if followed by any whitespace (including space at end) or at the end of the string
                    buffer[bytes_to_copy++] = (unsigned char)DEFAULT_END;
                }
            }

            memset(buffer + bytes_to_copy, (unsigned char)pad_id, max_length - bytes_to_copy);

            PyObject* py_word = PyBytes_FromStringAndSize((char*)buffer, max_length);
            if (!py_word || PyList_Append(list, py_word) < 0) {
                Py_XDECREF(py_word);
                Py_DECREF(list);
                return NULL;
            }
            Py_DECREF(py_word);

            word_start = it + 1; // Move to the start of the next word
        }
    }

    return list;
}

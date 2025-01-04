# cython: boundscheck=False, wraparound=False, nonecheck=False

import json
from random import Random, randrange, choice


cpdef list generate_ngrams(str text, int n, set valid_set):
    """Generate overlapping n-grams of size `n` from the text."""
    cdef list ngrams = []
    cdef int i, j, length = len(text)
    cdef str ngram
    cdef bint is_valid

    for i in range(length - n + 1):
        ngram = text[i:i + n].lower()
        is_valid = True
        for j in range(len(ngram)):
            if ngram[j] not in valid_set:
                is_valid = False
                break
        if is_valid:
            ngrams.append(ngram)

    return ngrams


cdef class MarkovModel:
    cdef public dict model  # The Markov model dictionary
    cdef public int depth   # Depth of the Markov chain
    cdef public set valid_set  # Set of valid characters

    def __init__(self, int depth=3, dict model=None, set valid_set=None):
        """
        Initialize the MarkovModel with a specified depth, optional pre-built model, 
        and custom valid set.

        Parameters:
            depth (int): Depth of the Markov chain.
            model (dict): Optional pre-built Markov model.
            valid_set (set): Set of valid characters for filtering n-grams.
        """
        self.depth = depth
        self.model = model if model is not None else {}
        self.valid_set = valid_set if valid_set is not None else set("abcdefghijklmnopqrstuvwxyz ")

    cpdef build(self, str text, bint update=True):
        """
        Build or update the Markov model from the given text.

        Args:
            text (str): text to process into the model
            update (bool): if update is set to false, a new model will be built.
        """
        cdef str ngram, next_char
        cdef list ngrams = generate_ngrams(text, self.depth, self.valid_set)

        for i in range(len(ngrams) - 1):
            ngram = ngrams[i]
            next_char = text[i + self.depth].lower()

            if ngram not in self.model:
                self.model[ngram] = {}

            if next_char in self.model[ngram]:
                self.model[ngram][next_char] += 1
            else:
                self.model[ngram][next_char] = 1

    cpdef str add_noise(self, str text, float probability, int seed=22):
        """
        Add random Markov noise to the input text.

        Args:
            text (str): Input text to modify.
            probability (float): Probability of replacing each n-gram.
            seed (int): Seed for the random number generator.

        Returns:
            str: The text with added Markov noise.
        """
        rng = Random(seed)
        cdef list output = []
        cdef str ngram, next_char
        cdef dict transitions
        cdef int target, cumulative, i
        cdef int length = len(text)

        if length < self.depth:
            return text

        ngram = text[:self.depth]
        output.append(ngram)

        for i in range(self.depth, length):
            if rng.random() < probability and ngram in self.model:
                transitions = self.model[ngram]
                if transitions:
                    target = rng.randrange(sum(transitions.values()))
                    cumulative = 0
                    for next_char, count in transitions.items():
                        cumulative += count
                        if cumulative > target:
                            output.append(next_char)
                            break
                else:
                    output.append(text[i])
            else:
                output.append(text[i])

            # Update the n-gram
            ngram = (ngram + text[i])[-self.depth:]

        return ''.join(output)

    def save(self, str filepath):
        """
        Save the Markov model, depth, and valid set to a JSON file.

        Args:
            filepath (str): Path to the file where the model should be saved.
        """
        data = {
            "depth": self.depth,
            "valid_set": list(self.valid_set),
            "model": self.model
        }
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False)

    @staticmethod
    def load(str filepath):
        """
        Load a Markov model, depth, and valid set from a JSON file.

        Args:
            filepath (str): Path to the file containing the saved model.

        Returns:
            MarkovModel: A new instance of MarkovModel initialized with the loaded data.
        """
        with open(filepath, "r", encoding="utf-8") as f:
            data = json.load(f)

        model = MarkovModel(
            depth=data["depth"],
            model=data["model"],
            valid_set=set(data["valid_set"])
        )
        return model


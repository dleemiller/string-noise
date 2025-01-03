import gzip
import json
import random
import re
import importlib.resources

from ..normalize import normalize
from ..noisers import (
    augment_string,
    augment_batch,
    SHUFFLE,
    RESHUFFLE,
    ASCENDING,
    DESCENDING
)


class Mapper:
    def __init__(self, json_data):
        self._json_data = normalize(json_data, debug=False)
        if not self._validate():
            raise ValueError("Invalid JSON structure")

    @property
    def map(self):
        return self._json_data

    @staticmethod
    def _is_valid_key(key):
        return isinstance(key, str)

    @staticmethod
    def _is_valid_value(value):
        return isinstance(value, (str, list, dict))

    def _validate(self):
        if not isinstance(self._json_data, dict):
            return False

        for key, value in self._json_data.items():
            if not self._is_valid_key(key) or not self._is_valid_value(value):
                return False

        return True

    @classmethod
    def load(cls, resource_package, resource_path):
        if resource_package is None:
            assert os.path.exists(resource_path)
            with open(resource_path, "r") as file:
                json_data = json.load(file)
        else:
            resource = importlib.resources.files(resource_package) / resource_path
            # Check if the file is gzipped
            if resource_path.endswith(".gz"):
                with gzip.open(resource, "rt") as file:
                    json_data = json.load(file)
            else:
                with resource.open("r") as file:
                    json_data = json.load(file)
        return cls(json_data)

    def __call__(
        self,
        text: str | list[str],
        probability: float = 1.0,
        sort_order=ASCENDING,
        debug=False,
        seed=None,
    ):
        if isinstance(text, str):
            return augment_string(
                text,
                self.map,
                probability=probability,
                debug=debug,
                sort_order=sort_order,
                seed=-1 if seed is None else seed,
            )
        elif isinstance(text, list):
            return augment_batch(
                text,
                self.map,
                probability=probability,
                debug=debug,
                sort_order=sort_order,
                seed=-1 if seed is None else seed,
            )
        else:
            raise TypeError("Input text must be `str` or list[str].")


class DictMapper(Mapper):
    def __init__(self, json_data):
        """
        Initialize the mapper with a dictionary where keys are correct words
        and values are lists of possible replacements (misspellings).
        """
        super().__init__(json_data)
        self._mapping = json_data

    @property
    def mapping(self):
        return self._mapping

    def random_replace(self, text, probability=0.5, seed=None):
        """
        Randomly replaces words in the input text based on the provided mapping.
        """
        if seed is not None:
            random.seed(seed)

        def replace_word(match):
            word = match.group(0)
            if random.random() < probability:
                lower_word = word.lower()
                choices = self.mapping.get(lower_word, [])
                if choices:
                    choice = random.choice(choices)
                    return self.match_case(word, choice)
            return word

        return re.sub(r"\b\w+\b", replace_word, text)

    @staticmethod
    def match_case(original, new_word):
        """
        Matches the case of the new word to the case of the original word.
        """
        if original.isupper():
            return new_word.upper()
        elif original.istitle():
            return new_word.title()
        return new_word.lower()

    def __call__(self, text: str, probability: float = 1.0, debug=False, seed=None):
        """
        Allows the object to be called like a function for convenience.
        """
        return self.random_replace(text, probability, seed)


# class TrieMapper(Mapper):
#    def __init__(self, json_data):
#        super().__init__(json_data)
#        self.__tree = Trie()
#        self.__tree.load(json_data)
#
#    @property
#    def tree(self):
#        return self.__tree
#
#    def random_replace(self, text, probability=0.5, seed=None):
#        if seed is not None:
#            random.seed(seed)
#
#        def replace_word(match):
#            word = match.group(0)
#            if random.random() < probability:
#                lower_word = word.lower()
#                choices = self.tree.lookup(lower_word) or []
#                if choices:
#                    choice = random.choice(choices)
#                    return self.match_case(word, choice)
#            return word
#
#        return re.sub(r"\b\w+\b", replace_word, text)
#
#    @staticmethod
#    def match_case(original, new_word):
#        if original.isupper():
#            return new_word.upper()
#        elif original.istitle():
#            return new_word.title()
#        return new_word.lower()
#
#    def __call__(self, text: str, probability: float = 1.0, debug=False, seed=None):
#        return self.random_replace(text, probability, seed)

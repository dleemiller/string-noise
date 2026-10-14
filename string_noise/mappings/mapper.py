import gzip
import json
import os
import re
import importlib.resources

from ..edits import Corruption, Edit, with_kind
from ..normalize import normalize
from ..noisers import (
    augment_string,
    augment_batch,
    make_rng,
    SHUFFLE,
    RESHUFFLE,
    ASCENDING,
    DESCENDING
)


def _open_text(source, gzipped, is_local):
    if gzipped:
        return gzip.open(source, "rt", encoding="utf-8")
    if is_local:
        return open(source, "r", encoding="utf-8")
    return source.open("r")


class Mapper:
    def __init__(self, json_data, kind=None):
        self._json_data = normalize(json_data, debug=False)
        self.kind = kind
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
    def load(cls, resource_package, resource_path, kind=None):
        """
        Load a mapping from package data, or from a local file when
        `resource_package` is None. Files ending in .gz are decompressed.
        `kind` names the noise in reported edits (default: the file's base name).
        """
        gzipped = str(resource_path).endswith(".gz")
        is_local = resource_package is None
        if is_local:
            if not os.path.exists(resource_path):
                raise FileNotFoundError(resource_path)
            source = resource_path
        else:
            source = importlib.resources.files(resource_package) / resource_path
        with _open_text(source, gzipped, is_local) as file:
            json_data = json.load(file)
        if kind is None:
            kind = os.path.basename(str(resource_path)).split(".")[0]
        return cls(json_data, kind=kind)

    def __call__(
        self,
        text: str | list[str],
        probability: float = 1.0,
        sort_order=ASCENDING,
        debug=False,
        seed=None,
        rng=None,
        return_edits=False,
    ):
        """
        Apply the mapping. With `return_edits`, returns (noisy, edits) where
        edits are raw (start, end, original, replacement, weight) tuples;
        use `corrupt` for tagged `Edit`s.
        """
        if isinstance(text, str):
            return augment_string(
                text,
                self.map,
                probability=probability,
                debug=debug,
                sort_order=sort_order,
                seed=-1 if seed is None else seed,
                rng=rng,
                return_edits=return_edits,
            )
        elif isinstance(text, list):
            return augment_batch(
                text,
                self.map,
                probability=probability,
                debug=debug,
                sort_order=sort_order,
                seed=-1 if seed is None else seed,
                rng=rng,
                return_edits=return_edits,
            )
        else:
            raise TypeError("Input text must be `str` or list[str].")

    def corrupt(
        self,
        text: str,
        probability: float = 1.0,
        sort_order=RESHUFFLE,
        seed=None,
        rng=None,
    ) -> Corruption:
        """Corrupt `text` and report every replacement as an `Edit` tagged with this mapper's kind."""
        noisy, edits = self(
            text,
            probability=probability,
            sort_order=sort_order,
            seed=seed,
            rng=rng,
            return_edits=True,
        )
        return Corruption(text, noisy, with_kind(edits, self.kind), self.kind)


_COUNT_SUFFIX = re.compile(r"^(.*)\*(\d+)$")


def _parse_variants(value):
    """
    Turn a misspelling entry into {variant: weight} summing to 1.

    Accepts a list (duplicates add weight, Norvig-style "word*N" adds N),
    a {variant: weight} dict, or a single string.
    """
    if isinstance(value, str):
        value = [value]
    if isinstance(value, dict):
        items = value.items()
    else:
        items = []
        for v in value:
            m = _COUNT_SUFFIX.match(v)
            items.append((m.group(1), int(m.group(2))) if m else (v, 1))
    counts = {}
    for variant, w in items:
        counts[variant] = counts.get(variant, 0.0) + float(w)
    total = sum(counts.values())
    if total <= 0:
        return {}
    return {k: v / total for k, v in counts.items()}


class DictMapper(Mapper):
    def __init__(self, json_data, kind=None):
        """
        Initialize the mapper with a dictionary where keys are correct words
        and values are lists of possible replacements (misspellings).
        """
        super().__init__(json_data, kind=kind)
        self._mapping = json_data
        self._variants = {}

    @property
    def mapping(self):
        return self._mapping

    def variants(self, word: str) -> dict:
        """
        All known misspellings of `word` (lowercased) with their probabilities,
        e.g. {"forer": 0.07, "fore": 0.36, ...}. Empty if the word is unknown.
        """
        key = word.lower()
        if key not in self._variants:
            raw = self._mapping.get(key)
            self._variants[key] = _parse_variants(raw) if raw else {}
        return self._variants[key]

    def _choose(self, word, rng):
        options = self.variants(word)
        if not options:
            return None, None
        choices = list(options)
        choice = rng.choices(choices, weights=[options[c] for c in choices])[0]
        return choice, options[choice]

    def corrupt(self, word: str, seed=None, rng=None) -> Corruption:
        """
        Replace a single word by one of its misspellings, chosen by weight.
        Returns an unchanged Corruption (no edits) if the word has no variants.
        """
        rng = make_rng(-1 if seed is None else seed, rng)
        choice, weight = self._choose(word, rng)
        if choice is None:
            return Corruption(word, word, (), self.kind)
        noisy = self.match_case(word, choice)
        edit = Edit(0, len(word), word, noisy, weight, self.kind)
        return Corruption(word, noisy, (edit,), self.kind)

    def random_replace(self, text, probability=0.5, seed=None, rng=None, return_edits=False):
        """
        Randomly replaces words in the input text based on the provided mapping.
        With `return_edits`, returns (noisy, edits) with `Edit`s in input coordinates.
        """
        rng = make_rng(-1 if seed is None else seed, rng)
        edits = []

        def replace_word(match):
            word = match.group(0)
            if rng.random() < probability:
                choice, weight = self._choose(word, rng)
                if choice is not None:
                    noisy = self.match_case(word, choice)
                    edits.append(
                        Edit(match.start(), match.end(), word, noisy, weight, self.kind)
                    )
                    return noisy
            return word

        out = re.sub(r"\b\w+\b", replace_word, text)
        if return_edits:
            return out, tuple(edits)
        return out

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

    def __call__(self, text: str, probability: float = 1.0, debug=False, seed=None,
                 rng=None, return_edits=False):
        """
        Allows the object to be called like a function for convenience.
        """
        return self.random_replace(text, probability, seed, rng=rng, return_edits=return_edits)

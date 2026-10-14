"""
Word-level corruption: pick one noise source per word from a weighted mixture.

Built for evaluation and training sets, where each item is one target word
corrupted by a single, labelled kind of noise:

    >>> from string_noise import noise, NoiseMixture
    >>> mix = NoiseMixture({"moe": (noise.moe, 0.7), "keyboard": (noise.keyboard, 0.3)})
    >>> c = mix.corrupt("because", seed=1)
    >>> c.noisy, c.kind, c.edits
"""

import inspect

from .edits import Corruption
from .noisers import make_rng


class NoiseMixture:
    def __init__(self, components: dict, probability: float = 1.0):
        """
        components: {kind: (noiser, weight)} where noiser has
            `corrupt(word, rng=...) -> Corruption`. Mapper noisers also take
            `probability` (per-character chance of a replacement), passed through
            from `probability` here.
        """
        if not components:
            raise ValueError("components must not be empty")
        for kind, (noiser, weight) in components.items():
            if not hasattr(noiser, "corrupt"):
                raise TypeError(f"noiser for {kind!r} has no corrupt()")
            if weight < 0:
                raise ValueError(f"negative weight for {kind!r}")
        self.components = dict(components)
        self.probability = probability
        self._takes_probability = {
            kind: "probability" in inspect.signature(noiser.corrupt).parameters
            for kind, (noiser, _) in self.components.items()
        }

    def _corrupt_with(self, kind, noiser, word, rng):
        if self._takes_probability[kind]:
            c = noiser.corrupt(word, probability=self.probability, rng=rng)
        else:
            c = noiser.corrupt(word, rng=rng)
        return Corruption(c.original, c.noisy, tuple(e._replace(kind=kind) for e in c.edits), kind)

    def corrupt(self, word: str, seed=None, rng=None) -> Corruption:
        """
        Corrupt `word` with one component drawn by weight. If the drawn
        component cannot change the word (e.g. no known misspelling), the
        remaining components are tried in weighted order. Returns an unchanged
        Corruption with kind None if none of them changes it.
        """
        rng = make_rng(-1 if seed is None else seed, rng)
        remaining = {k: v for k, v in self.components.items() if v[1] > 0}
        while remaining:
            kinds = list(remaining)
            kind = rng.choices(kinds, weights=[remaining[k][1] for k in kinds])[0]
            noiser, _ = remaining.pop(kind)
            c = self._corrupt_with(kind, noiser, word, rng)
            if c.changed:
                return c
        return Corruption(word, word, (), None)

"""
Edit provenance for noise operations.

Noisers that support `return_edits` / `corrupt(...)` report exactly what they
changed, so evaluation sets can be labelled by noise type and position.
"""

from dataclasses import dataclass
from typing import NamedTuple, Optional


class Edit(NamedTuple):
    """
    One replacement, in coordinates of the ORIGINAL string.

    - start, end: span replaced in the original (end exclusive; start == end for an insertion)
    - original: the replaced text (original[start:end])
    - replacement: the text that took its place
    - weight: normalized probability of this replacement among the options
      for `original` in the mapping, or None when not defined
    - kind: name of the noiser that produced it (e.g. "moe", "keyboard")
    """

    start: int
    end: int
    original: str
    replacement: str
    weight: Optional[float] = None
    kind: Optional[str] = None


@dataclass(frozen=True)
class Corruption:
    """Result of corrupting a string: the original, the noisy output and its edits."""

    original: str
    noisy: str
    edits: tuple
    kind: Optional[str] = None

    @property
    def changed(self) -> bool:
        return self.noisy != self.original


def apply_edits(text: str, edits) -> str:
    """
    Rebuild the noisy string from `text` and non-overlapping `edits`
    given in original-string coordinates.
    """
    out = []
    pos = 0
    for e in sorted(edits, key=lambda e: (e.start, e.end)):
        if e.start < pos:
            raise ValueError(f"overlapping edit at {e.start}")
        if text[e.start : e.end] != e.original:
            raise ValueError(
                f"edit original {e.original!r} != text[{e.start}:{e.end}] {text[e.start:e.end]!r}"
            )
        out.append(text[pos : e.start])
        out.append(e.replacement)
        pos = e.end
    out.append(text[pos:])
    return "".join(out)


def with_kind(edits, kind):
    """Tag raw (start, end, original, replacement, weight) tuples as Edits of `kind`."""
    return tuple(Edit(*e, kind=kind) for e in edits)

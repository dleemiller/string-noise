import functools

from .mapper import Mapper

RESOURCE_PACKAGE = "string_noise.mappings.default"


class CustomMapper:
    @classmethod
    def new(cls, path):
        return functools.partial(
            Mapper.load, resource_package=RESOURCE_PACKAGE, resource_path=path
        )


load_leet = CustomMapper.new("leet.json")
load_homoglyph = CustomMapper.new("homoglyphs.json")
load_phonetic = CustomMapper.new("phonetic.json")
load_keyboard = CustomMapper.new("keyboard.json")

__all__ = [
    "CustomMapper",
    "load_leet",
    "load_homoglyph",
    "load_phonetic",
    "load_keyboard",
]

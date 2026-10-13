import functools

from .mapper import Mapper, DictMapper

RESOURCE_PACKAGE = "string_noise.mappings.default"


class CustomMapper:
    @classmethod
    def new(cls, path, resource_package=RESOURCE_PACKAGE, kind=None):
        """
        Set resource_package to `None` for custom local mappings.
        `kind` names the noise in reported edits (default: the file's base name).
        """
        return functools.partial(
            Mapper.load, resource_package=resource_package, resource_path=path, kind=kind
        )

class CustomDictMapper:
    @classmethod
    def new(cls, path, resource_package=RESOURCE_PACKAGE, kind=None):
        """
        Set resource_package to `None` for custom local mappings.
        `kind` names the noise in reported edits (default: the file's base name).
        """
        return functools.partial(
            DictMapper.load, resource_package=resource_package, resource_path=path, kind=kind
        )


load_leet = CustomMapper.new("leet.json", kind="leet")
load_homoglyph = CustomMapper.new("homoglyphs.json", kind="homoglyph")
load_phonetic = CustomMapper.new("phonetic.json", kind="phonetic")
load_keyboard = CustomMapper.new("keyboard.json", kind="keyboard")
load_llm_ocr = CustomMapper.new("llm_ocr.json", kind="basic_ocr")
load_nguyen_ocr = CustomMapper.new("nguyen_ocr.json", kind="ocr")

load_mispelling = CustomDictMapper.new("spell_errors.json", kind="mispelling")
load_moe_mispelling = CustomDictMapper.new("moe_reduced.json.gz", kind="moe")


__all__ = [
    "Mapper",
    "DictMapper",
    "CustomMapper",
    "CustomDictMapper",
    "load_leet",
    "load_homoglyph",
    "load_phonetic",
    "load_keyboard",
    "load_llm_ocr",
    "load_nguyen_ocr",
    "load_mispelling",
    "load_moe_mispelling",
]

import functools

from .mapper import Mapper, DictMapper

RESOURCE_PACKAGE = "string_noise.mappings.default"


class CustomMapper:
    @classmethod
    def new(cls, path, resource_package=RESOURCE_PACKAGE):
        """
        Set resource_package to `None` for custom local mappings.
        """
        return functools.partial(
            Mapper.load, resource_package=resource_package, resource_path=path
        )

class CustomDictMapper:
    @classmethod
    def new(cls, path, resource_package=RESOURCE_PACKAGE):
        """
        Set resource_package to `None` for custom local mappings.
        """
        return functools.partial(
            DictMapper.load, resource_package=resource_package, resource_path=path
        )


load_leet = CustomMapper.new("leet.json")
load_homoglyph = CustomMapper.new("homoglyphs.json")
load_phonetic = CustomMapper.new("phonetic.json")
load_keyboard = CustomMapper.new("keyboard.json")
load_llm_ocr = CustomMapper.new("llm_ocr.json")
load_nguyen_ocr = CustomMapper.new("nguyen_ocr.json")

load_mispelling = CustomDictMapper.new("spell_errors.json")
load_moe_mispelling = CustomDictMapper.new("moe_reduced.json.gz")


__all__ = [
    "CustomMapper",
    "load_leet",
    "load_homoglyph",
    "load_phonetic",
    "load_keyboard",
    "load_llm_ocr",
    "load_nguyen_ocr",
    "load_mispelling",
    "load_moe_mispelling",
]

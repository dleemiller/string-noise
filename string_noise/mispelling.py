import json
import re
import random
from string_noise.string_noise import lookup, build_tree  # C extension function
from .mappings import RESOURCE_PACKAGE as resource_package
import importlib


def _build_default_tree(resource_path="spell_errors.json"):
    # Use importlib.resources.files() to access the resource
    resource = importlib.resources.files(resource_package) / resource_path
    with resource.open("r") as file:
        json_data = json.load(file)
    build_tree(json_data)


def random_mispelling(text, probability=0.5, seed=None):
    if seed is not None:
        random.seed(seed)
    cache = {}

    def replace_word(match):
        word = match.group(0)
        if random.random() < probability:
            lower_word = word.lower()
            if lower_word in cache:
                misspellings = cache[lower_word]
            else:
                misspellings = lookup(lower_word) or []
                cache[lower_word] = misspellings

            if misspellings:
                misspelling = random.choice(misspellings)
                return match_case(word, misspelling)
        return word

    return re.sub(r'\b\w+\b', replace_word, text)

def match_case(original, new_word):
    if original.isupper():
        return new_word.upper()
    elif original.istitle():
        return new_word.title()
    return new_word.lower()

# Example usage
#modified_text = random_mispelling("Your example text here", 0.5)
#print(modified_text)


# string-noise

The `string-noise` Python package provides a suite of tools for augmenting strings through various noise addition techniques. It's designed to assist in creating synthetic datasets, particularly beneficial for training and evaluating machine learning models. The package offers a range of augmentation styles, including:

- Common Misspellings
- Markov Chain-Based Alterations
- OCR (Optical Character Recognition) Simulations
- Character Masking
- Homoglyph Replacements
- And more!

Key Features:
- Implements an N:M character-sequence mapping for diverse augmentation possibilities.
- Probabilistic and weighted replacements provide realistic and varied text distortions.
- Optimized for performance with C extensions, ensuring fast processing even for large datasets.
- Utilizes a Trie data structure in C, enabling efficient handling of extensive maps (such as those required for common misspellings and Markov chains).
- Designed for ease of use, the package has a straightforward API and requires no external dependencies.


## Table of Contents
1. [Getting Started](#getting-started)
2. [Quickstart](#quickstart)
3. [API Walkthrough](#api-walkthrough)
4. [Contributing](#contributing)
5. [License](#license)

## Getting Started

### Installation
To install the package, use the following command:
```bash
python setup.py build
python setup.py install
```
Ensure you have Python 3.x installed on your system.

### Quickstart
To begin using `string-noise`, import the `noise` object and use its methods to add noise to your strings. Here's an example:

```python
from string_noise import noise

# Applying random noise to a string
result = noise.random("Hello world!", probability=0.5, seed=42)
print(result)  # Output might be 'H3llo wor7d!', depending on the random seed
```

## API Walkthrough

The `noise` object, an instance of the `LazyNoise` class, is your primary interface for applying noise to strings.

### 1. `random`
Add random noise to a string using a specified charset.

- `original_string (str)`: The string to apply random noise to.
- `charset (str)`: Optional; characters to use as replacements (default: ASCII letters and digits).
- `min_chars_in (int)`: Optional; minimum number of consecutive characters to consider for replacement (default: 1).
- `max_chars_in (int)`: Optional; maximum number of consecutive characters to consider for replacement (default: 2).
- `min_chars_out (int)`: Optional; minimum number of replacement characters (default: 1).
- `max_chars_out (int)`: Optional; maximum number of replacement characters (default: 2).
- `probability (float)`: Optional; probability of any character being replaced (0-1) (default: 0.1).
- `seed (int)`: Optional; seed for the random number generator, -1 for random (default: -1).
- `debug (bool)`: Optional; enables debug output if set to True (default: False).

### 2. Predefined Noise Mappers
These methods apply predefined noise patterns based on various styles:

#### `ocr`
Mimics OCR-related noise, based on the analysis of common OCR errors. Example usage:

#### `basic_ocr`
A simplified version of OCR noise, faster, suitable for less complex noise requirements. Example usage:

#### `homoglyph`
Replace characters with similar-looking Unicode glyphs. Example output with `probability=0.3`:
```
Lorem ip5υɱ ԁoloя sit ame7, ¢onsectetυř ªd!ρіscіng еlit, ßed dö eiusmоd τеmpor inсidіdunt u+ la6øre eτ cloloя3 nnаgna аligμ@.
```

#### `leet`
Apply leet (1337) replacements to characters. Example output with `probability=0.3`:
```
|o|23m ips|_|m dolo|2 sit @m3t, co|\\|$3ctetur ad!piscing e1i7, 5ed d0 eiusmo|] tempo|2 incid!dunt |_|t labo.-e et |)olore m46na a|1qua.
```

#### `phonetic`
Replace characters based on similar sounds.

#### `keyboard`
Simulate nearest keystroke mistakes.

#### `mispelling`
Small mispelling lookup for lighter mispelling noising.

#### `moe`
Uses top 167k words from the "Misspelling Oblivious Word Embeddings" corpus: https://arxiv.org/abs/1905.09755

Fast performance using Trie data structure implemented in C.


Each of these methods accepts the following parameters:

- `input_string (str)`: The string to apply the noise to.
- `probability (float)`: Optional; the probability of each character or substring being replaced (0-1).
- `seed (int)`: Optional; seed for the random number generator, -1 for random.
- `sort_order (int)`: Optional; order to process replacement keys (SHUFFLE, RESHUFFLE, ASCENDING, DESCENDING). RESHUFFLE sorts the map keys on every iteration, while others only at the start.

Example usage:
```python
result = noise.homoglyph("Hello world", probability=0.3, seed=42, sort_order=SHUFFLE)
print(result)  # Output with specified parameters
```

### 3. `mask`
Apply random masking to a string, selectively replacing characters based on their type (vowel, consonant, etc.) and Unicode byte size.

- `input_string (str)`: The string to apply masking to.
- `probability (float)`: Optional; probability of each character being masked (0-1) (default: 0.1).
- `min_consecutive (int)`: Optional; minimum number of consecutive characters to consider for masking (default: 1).
- `max_consecutive (int)`: Optional; maximum number of consecutive characters to consider for masking (default: 2).
- `vowel_mask (int)`: Optional; mask value for vowels (default: 0x06).
- `consonant_mask (int)`: Optional; mask value for consonants (default: 0x07).
- `nws_mask (int)`: Optional; mask value for non-whitespace characters (default: 0x08).
- `general_mask (int)`: Optional; general mask value for characters (default: 0x09).
- `two_byte_mask (int)`: Optional; mask value for 2-byte Unicode characters (default: 0x0A).
- `four_byte_mask (int)`: Optional; mask value for 4-byte Unicode characters (default: 0x0B).
- `general_mask_probability (float)`: Optional; probability of using the general mask instead of specific masks (0-1) (default: 0.5).
- `seed (int)`: Optional; seed for the random number generator, -1 for random (default: -1).
- `skip_digits (bool)`: Optional; toggles digit masking on and off (default: False)
- `debug (bool)`: Optional; enables debug output if set to True (default: False).

Example usage:
```python
from string_noise import noise

# Applying random masking to a string
masked_result = noise.mask("Hello world!", probability=0.5, seed=42)
print(masked_result)  # Output might be 'H\x06\x07l\x06 \x09\x07rld!', depending on the random seed
```

## Contributing
Contributions to `string-noise` are welcome! Please refer to the project's issues and pull request sections on [GitHub](https://github.com/your-repository-url).

## License
`string-noise` is licensed under the [MIT License](https://opensource.org/licenses/MIT).



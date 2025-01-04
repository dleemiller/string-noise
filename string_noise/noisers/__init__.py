from .mask import random_masking, random_mask_batch, DefaultMask
from .augment import augment_string, augment_batch, SortOrder
from .random import random_replacement
from .markov import MarkovModel

SHUFFLE = SortOrder.SHUFFLE
RESHUFFLE = SortOrder.RESHUFFLE
ASCENDING = SortOrder.ASCENDING
DESCENDING = SortOrder.DESCENDING

DEFAULT_VOWEL_MASK = DefaultMask.VOWEL
DEFAULT_CONSONANT_MASK = DefaultMask.CONSONANT
DEFAULT_DIGIT_MASK = DefaultMask.DIGIT
DEFAULT_NWS_MASK = DefaultMask.NWS
DEFAULT_GENERAL_MASK = DefaultMask.GENERAL
DEFAULT_2BYTE_MASK = DefaultMask.TWO_BYTE
DEFAULT_3BYTE_MASK = DefaultMask.THREE_BYTE
DEFAULT_4BYTE_MASK = DefaultMask.FOUR_BYTE

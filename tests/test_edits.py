import gzip
import json
import os
import random
import tempfile
import unittest

from string_noise import (
    noise,
    apply_edits,
    Corruption,
    Edit,
    NoiseMixture,
    CustomMapper,
    CustomDictMapper,
    SHUFFLE,
    RESHUFFLE,
    ASCENDING,
)
from string_noise.normalize import normalize

TEXT = "the quick brown fox jumps over the lazy dog because they're there"


class TestSeedingAndSortOrders(unittest.TestCase):
    def test_shuffle_sort_order_runs(self):
        out = noise.keyboard(TEXT, probability=0.5, sort_order=SHUFFLE, seed=3)
        self.assertIsInstance(out, str)

    def test_same_seed_same_output(self):
        for order in (ASCENDING, SHUFFLE, RESHUFFLE):
            a = noise.keyboard(TEXT, probability=0.5, sort_order=order, seed=7)
            b = noise.keyboard(TEXT, probability=0.5, sort_order=order, seed=7)
            self.assertEqual(a, b)

    def test_global_random_state_untouched(self):
        random.seed(123)
        expected = random.random()
        random.seed(123)
        noise.keyboard(TEXT, probability=0.5, seed=1)
        noise.moe("because", seed=1)
        self.assertEqual(random.random(), expected)

    def test_shared_rng_stream(self):
        rng_a = random.Random(5)
        rng_b = random.Random(5)
        a = [noise.keyboard(TEXT, probability=0.3, rng=rng_a) for _ in range(3)]
        b = [noise.keyboard(TEXT, probability=0.3, rng=rng_b) for _ in range(3)]
        self.assertEqual(a, b)
        self.assertGreater(len(set(a)), 1)


class TestMapperEdits(unittest.TestCase):
    def test_edits_rebuild_output(self):
        for mapper in (noise.keyboard, noise.phonetic, noise.ocr):
            for seed in range(50):
                noisy, edits = mapper(
                    TEXT, probability=0.3, sort_order=RESHUFFLE, seed=seed, return_edits=True
                )
                as_edits = [Edit(*e) for e in edits]
                self.assertEqual(apply_edits(TEXT, as_edits), noisy)
                for e in as_edits:
                    self.assertEqual(TEXT[e.start : e.end], e.original)
                    self.assertTrue(e.weight is None or 0 < e.weight <= 1)

    def test_corrupt_tags_kind(self):
        c = noise.keyboard.corrupt("keyboard", probability=1.0, seed=2)
        self.assertIsInstance(c, Corruption)
        self.assertTrue(c.changed)
        self.assertTrue(all(e.kind == "keyboard" for e in c.edits))
        self.assertEqual(apply_edits(c.original, c.edits), c.noisy)

    def test_no_edits_when_probability_zero(self):
        c = noise.keyboard.corrupt("keyboard", probability=0.0, seed=2)
        self.assertFalse(c.changed)
        self.assertEqual(c.edits, ())

    def test_batch_return_edits(self):
        results = noise.keyboard(["hello", "world"], probability=0.5, seed=4, return_edits=True)
        for original, (noisy, edits) in zip(["hello", "world"], results):
            self.assertEqual(apply_edits(original, [Edit(*e) for e in edits]), noisy)


class TestNormalize(unittest.TestCase):
    def test_list_duplicates_keep_weight(self):
        out = normalize({"k": ["a", "a", "b"]})
        self.assertAlmostEqual(out["k"]["a"], 2 / 3)
        self.assertAlmostEqual(out["k"]["b"], 1 / 3)


class TestLocalMappings(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()

    def tearDown(self):
        self.tmp.cleanup()

    def _write(self, name, data, gz=False):
        path = os.path.join(self.tmp.name, name)
        opener = gzip.open if gz else open
        with opener(path, "wt", encoding="utf-8") as f:
            json.dump(data, f)
        return path

    def test_local_json_and_gz(self):
        for gz in (False, True):
            path = self._write("ie.json" + (".gz" if gz else ""), {"ie": "ei"}, gz=gz)
            mapper = CustomMapper.new(path, resource_package=None, kind="ie_swap")()
            c = mapper.corrupt("believe", probability=1.0, seed=0)
            self.assertEqual(c.noisy, "beleive")
            self.assertEqual(c.edits, (Edit(3, 5, "ie", "ei", 1.0, "ie_swap"),))

    def test_local_missing_file(self):
        with self.assertRaises(FileNotFoundError):
            CustomMapper.new(os.path.join(self.tmp.name, "nope.json"), resource_package=None)()

    def test_local_dict_mapper(self):
        path = self._write("words.json", {"there": ["their*3", "they're"]})
        mapper = CustomDictMapper.new(path, resource_package=None, kind="confusion")()
        self.assertEqual(mapper.variants("There"), {"their": 0.75, "they're": 0.25})


class TestDictMapper(unittest.TestCase):
    def test_count_suffix_parsed(self):
        variants = noise.mispelling.variants("four")
        self.assertTrue(variants)
        self.assertFalse(any("*" in v for v in variants))
        self.assertAlmostEqual(sum(variants.values()), 1.0)
        # "fore*5" outweighs single-count variants
        self.assertGreater(variants["fore"], variants["forer"])

    def test_no_count_suffix_in_output(self):
        words = [w for w, v in noise.mispelling.mapping.items() if any("*" in x for x in v)][:200]
        self.assertTrue(words)
        rng = random.Random(0)
        for w in words:
            c = noise.mispelling.corrupt(w, rng=rng)
            self.assertNotIn("*", c.noisy)

    def test_corrupt_word(self):
        c = noise.moe.corrupt("Because", seed=1)
        self.assertTrue(c.changed)
        self.assertTrue(c.noisy[0].isupper())
        (edit,) = c.edits
        self.assertEqual((edit.start, edit.end, edit.original), (0, 7, "Because"))
        self.assertEqual(edit.kind, "moe")
        self.assertAlmostEqual(edit.weight, noise.moe.variants("because")[c.noisy.lower()])

    def test_unknown_word_unchanged(self):
        c = noise.moe.corrupt("zzqxv", seed=1)
        self.assertFalse(c.changed)
        self.assertEqual(c.edits, ())

    def test_random_replace_edits(self):
        noisy, edits = noise.moe(TEXT, probability=0.5, seed=9, return_edits=True)
        self.assertTrue(edits)
        self.assertEqual(apply_edits(TEXT, edits), noisy)


class TestNoiseMixture(unittest.TestCase):
    def setUp(self):
        self.mix = NoiseMixture(
            {"moe": (noise.moe, 0.7), "keyboard": (noise.keyboard, 0.3)}, probability=0.3
        )

    def test_labels_and_rebuild(self):
        rng = random.Random(0)
        kinds = set()
        for _ in range(200):
            c = self.mix.corrupt("because", rng=rng)
            self.assertTrue(c.changed)
            kinds.add(c.kind)
            self.assertTrue(all(e.kind == c.kind for e in c.edits))
            self.assertEqual(apply_edits(c.original, c.edits), c.noisy)
        self.assertEqual(kinds, {"moe", "keyboard"})

    def test_falls_back_when_component_cannot_change_word(self):
        c = NoiseMixture(
            {"moe": (noise.moe, 1.0), "keyboard": (noise.keyboard, 1e-9)}, probability=1.0
        ).corrupt("zzqxv", seed=0)
        self.assertEqual(c.kind, "keyboard")

    def test_deterministic(self):
        self.assertEqual(self.mix.corrupt("because", seed=11), self.mix.corrupt("because", seed=11))

    def test_unchangeable_word(self):
        c = NoiseMixture({"moe": (noise.moe, 1.0)}).corrupt("zzqxv", seed=0)
        self.assertFalse(c.changed)
        self.assertIsNone(c.kind)


if __name__ == "__main__":
    unittest.main()

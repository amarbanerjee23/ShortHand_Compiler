#!/usr/bin/env python3
import itertools
import unittest

import pr_runtime_delta as delta


class PrRuntimeDeltaTest(unittest.TestCase):
    def test_orders_are_balanced(self):
        rows = delta.balanced_orders(112, 6)
        self.assertEqual(len(rows), 6)
        self.assertEqual(sum(row == ["head", "base"] for row in rows), 3)
        self.assertEqual(sum(row == ["base", "head"] for row in rows), 3)

    def test_invalid_block_design_rejected(self):
        for blocks in (0, 3, 22):
            with self.assertRaises(ValueError):
                delta.balanced_orders(1, blocks)

    def test_summary_uses_paired_ratios(self):
        rows = [
            {"head_us_per_image": 8.0, "base_us_per_image": 10.0, "ratio": 0.8},
            {"head_us_per_image": 9.0, "base_us_per_image": 10.0, "ratio": 0.9},
            {"head_us_per_image": 7.0, "base_us_per_image": 10.0, "ratio": 0.7},
            {"head_us_per_image": 8.5, "base_us_per_image": 10.0, "ratio": 0.85},
        ]
        out = delta.summarize_cell(rows)
        self.assertEqual(out["blocks"], 4)
        self.assertAlmostEqual(out["paired_ratio_median"], 0.825)
        self.assertAlmostEqual(out["paired_delta_percent_median"], 17.5)
        self.assertEqual(out["paired_ratio_min"], 0.7)
        self.assertEqual(out["paired_ratio_max"], 0.9)

    def test_equivalence_allows_revision_difference_only(self):
        q = {"absolute_tolerance": 1e-5, "relative_tolerance": 1e-4}
        common = {
            "success": True,
            "configuration_sha256": "a",
            "qualification_sha256": "b",
            "model_sha256": "c",
            "dataset_sha256": "d",
            "dataset_id": "id",
            "dataset_split": "test",
            "precision": "float32",
            "backend": "onnxruntime_cpu",
            "backend_version": "1.30.0",
            "threads": 1,
            "batch_size": 16,
            "functional_unit": "completed_classification",
            "rows": 2,
            "minimum_accuracy": 0.5,
            "accuracy": 1.0,
            "predictions": [1, 0],
            "top_k": [1, 0, 2, 0, 1, 2],
            "scores": [0.0, 1.0, -1.0, 1.0, 0.0, -1.0],
            "trials": [
                {"success": True, "completed": 4, "elapsed_ms": 1.0},
                {"success": True, "completed": 4, "elapsed_ms": 1.1},
                {"success": True, "completed": 4, "elapsed_ms": 0.9},
            ],
        }
        head = dict(common, compiler_revision="head")
        base = dict(common, compiler_revision="base")
        delta.validate_equivalence(head, base, q)
        bad = dict(base, predictions=[0, 0])
        with self.assertRaises(ValueError):
            delta.validate_equivalence(head, bad, q)

    def test_markdown_keeps_claim_boundary(self):
        summary = {
            "head_sha": "h",
            "base_sha": "b",
            "cells": [{
                "cell": "b16-t1",
                "head_median_us_per_image": 1.0,
                "base_median_us_per_image": 2.0,
                "paired_ratio_median": 0.5,
                "paired_delta_percent_median": 50.0,
                "paired_ratio_min": 0.4,
                "paired_ratio_max": 0.6,
            }],
        }
        text = delta.markdown(summary)
        self.assertIn("no hosted timing threshold", text)
        self.assertIn("Latency is not converted to joules", text)


if __name__ == "__main__":
    unittest.main()

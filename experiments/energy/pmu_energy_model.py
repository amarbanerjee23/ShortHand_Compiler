#!/usr/bin/env python3
"""Calibrated PMU-to-Joules Energy Model for CI & Software Energy Attribution.

Fits a non-negative ridge-regularized linear model mapping hardware performance
counters and elapsed time (seconds, cycles, instructions, LLC misses) from
qualified bare-metal RAPL+PMU runs to net energy (joules):

    E_hat = max(0, beta_0 * seconds + beta_1 * cycles + beta_2 * instructions + beta_3 * llc_misses)

Governance boundary:
  - Calibration profiles are host-family specific and stored under
    experiments/energy/calibration_profiles/.
  - Predictions ALWAYS emit evidence_class="calibrated_pmu_estimate" and
    energy_claim_authorized=False. They provide deterministic per-commit
    energy attribution and bottleneck diagnostics in CI, and NEVER substitute
    for bare-metal RAPL/AC-meter runs required by frozen_energy_protocol.py.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib
import sys
from typing import Any

SCHEMA = "shorthand.pmu_energy_model.v1"
FEATURES = ("seconds", "cycles", "instructions", "llc_misses")


def _solve_linear_system(a: list[list[float]], b: list[float]) -> list[float]:
    """Gaussian elimination with partial pivoting for small (4x4) dense systems."""
    n = len(b)
    m = [row[:] + [b[i]] for i, row in enumerate(a)]
    for col in range(n):
        pivot = max(range(col, n), key=lambda r: abs(m[r][col]))
        m[col], m[pivot] = m[pivot], m[col]
        if abs(m[col][col]) < 1e-18:
            raise ValueError("singular design matrix; add regularization or diverse samples")
        inv = 1.0 / m[col][col]
        for j in range(col, n + 1):
            m[col][j] *= inv
        for r in range(n):
            if r != col and m[r][col] != 0.0:
                factor = m[r][col]
                for j in range(col, n + 1):
                    m[r][j] -= factor * m[col][j]
    return [m[i][n] for i in range(n)]


def fit_model(samples: list[dict[str, float]], ridge_alpha: float = 1e-6) -> dict[str, Any]:
    """Fit a ridge-regularized linear energy model from PMU+RAPL observations."""
    if len(samples) < len(FEATURES):
        raise ValueError(f"need at least {len(FEATURES)} calibration samples, got {len(samples)}")
    if ridge_alpha <= 0:
        raise ValueError("ridge_alpha must be positive")

    # Feature scaling so seconds (~0.5) and instructions (~1e9) are well-conditioned.
    scales: dict[str, float] = {}
    for feat in FEATURES:
        vals = [float(s[feat]) for s in samples]
        if any(v < 0 or not math.isfinite(v) for v in vals):
            raise ValueError(f"feature {feat} must be finite and non-negative")
        mx = max(vals)
        scales[feat] = mx if mx > 0.0 else 1.0

    y = [float(s["net_joules"]) for s in samples]
    if any(v < 0 or not math.isfinite(v) for v in y):
        raise ValueError("net_joules must be finite and non-negative")

    x_rows = [[float(s[f]) / scales[f] for f in FEATURES] for s in samples]
    d = len(FEATURES)
    xtx = [[0.0] * d for _ in range(d)]
    xty = [0.0] * d
    for row, target in zip(x_rows, y):
        for i in range(d):
            xty[i] += row[i] * target
            for j in range(d):
                xtx[i][j] += row[i] * row[j]
    for i in range(d):
        xtx[i][i] += ridge_alpha

    theta_scaled = _solve_linear_system(xtx, xty)
    # Clamp tiny negative numerical artifacts and unscale coefficients.
    coefficients = {
        feat: max(0.0, theta_scaled[i] / scales[feat])
        for i, feat in enumerate(FEATURES)
    }

    preds = [predict_joules(coefficients, s)["estimated_joules"] for s in samples]
    residuals = [abs(p - actual) for p, actual in zip(preds, y)]
    mae = sum(residuals) / len(residuals)
    rmse = math.sqrt(sum((p - actual) ** 2 for p, actual in zip(preds, y)) / len(residuals))
    mean_y = sum(y) / len(y)
    ss_tot = sum((actual - mean_y) ** 2 for actual in y)
    ss_res = sum((p - actual) ** 2 for p, actual in zip(preds, y))
    r2 = 1.0 - (ss_res / ss_tot) if ss_tot > 0.0 else 1.0

    profile = {
        "schema": SCHEMA,
        "features": list(FEATURES),
        "coefficients": coefficients,
        "ridge_alpha": ridge_alpha,
        "sample_count": len(samples),
        "fit_metrics": {"mae_joules": mae, "rmse_joules": rmse, "r2": r2},
        "evidence_class": "calibrated_pmu_estimate",
        "energy_claim_authorized": False,
    }
    profile["profile_sha256"] = hashlib.sha256(
        json.dumps(coefficients, sort_keys=True).encode()
    ).hexdigest()
    return profile


def predict_joules(coefficients: dict[str, float], observation: dict[str, float]) -> dict[str, Any]:
    """Predict joules and per-component attribution from PMU counters."""
    breakdown: dict[str, float] = {}
    total = 0.0
    for feat in FEATURES:
        val = float(observation.get(feat, 0.0))
        if val < 0 or not math.isfinite(val):
            raise ValueError(f"observation {feat} must be finite and non-negative")
        contrib = max(0.0, float(coefficients.get(feat, 0.0)) * val)
        breakdown[feat] = contrib
        total += contrib
    return {
        "schema": SCHEMA,
        "estimated_joules": total,
        "component_attribution_joules": breakdown,
        "evidence_class": "calibrated_pmu_estimate",
        "energy_claim_authorized": False,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    f = sub.add_parser("fit", help="Fit calibration profile from JSON list of PMU+RAPL samples")
    f.add_argument("--samples", type=pathlib.Path, required=True)
    f.add_argument("--output", type=pathlib.Path, required=True)
    f.add_argument("--ridge-alpha", type=float, default=1e-6)

    p = sub.add_parser("predict", help="Predict estimated joules from a calibration profile")
    p.add_argument("--profile", type=pathlib.Path, required=True)
    p.add_argument("--observation", type=pathlib.Path, required=True)
    p.add_argument("--output", type=pathlib.Path)

    args = parser.parse_args(argv)
    if args.command == "fit":
        samples = json.loads(args.samples.read_text())
        profile = fit_model(samples, ridge_alpha=args.ridge_alpha)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(profile, indent=2, sort_keys=True) + "\n")
        print(json.dumps(profile, indent=2, sort_keys=True))
        return 0
    if args.command == "predict":
        profile = json.loads(args.profile.read_text())
        obs = json.loads(args.observation.read_text())
        res = predict_joules(profile["coefficients"], obs)
        res["profile_sha256"] = profile.get("profile_sha256")
        text = json.dumps(res, indent=2, sort_keys=True)
        if args.output:
            args.output.write_text(text + "\n")
        print(text)
        return 0
    return 2


if __name__ == "__main__":
    sys.exit(main())

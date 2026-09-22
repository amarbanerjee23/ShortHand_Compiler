"""Paired block analysis. Synthetic test values must never be reported as data."""
import math
import random
import statistics


def compare(native, python, seed=104, uncertainty_percent=0):
    if len(native) != len(python) or len(native) < 4:
        raise ValueError('at least four complete paired blocks required')
    if any(not math.isfinite(v) or v <= 0 for v in native + python):
        raise ValueError('positive finite observations required')
    if not 0 <= uncertainty_percent < 100:
        raise ValueError('invalid instrument uncertainty')
    rng = random.Random(seed)
    ratios = []
    for _ in range(10000):
        indices = rng.choices(range(len(native)), k=len(native))
        ratios.append(sum(native[i] for i in indices) / sum(python[i] for i in indices))
    ratios.sort()
    lower, upper = ratios[249], ratios[9749]
    ratio = statistics.mean(native) / statistics.mean(python)
    u = uncertainty_percent / 100
    # Keep sampling confidence and systematic instrument uncertainty distinct.
    expanded = [100 * (1 - upper * (1 + u) / (1 - u)),
                100 * (1 - lower * (1 - u) / (1 + u))]
    return dict(native_raw=native, python_raw=python, paired_blocks=len(native),
                native_mean=statistics.mean(native), python_mean=statistics.mean(python),
                native_over_python_ratio=ratio, savings_percent=100 * (1 - ratio),
                paired_bootstrap_95_percent_interval=[100 * (1 - upper), 100 * (1 - lower)],
                instrument_expanded_savings_range=expanded,
                interpretation=('reduction_observed' if expanded[0] > 0 else
                                'increase_observed' if expanded[1] < 0 else 'inconclusive'),
                method='10000 paired-block percentile bootstrap; within-session exploratory interval')


def break_even(compile_joules, native_run_joules, python_run_joules, units):
    if any(not math.isfinite(v) or v <= 0 for v in
           [compile_joules, native_run_joules, python_run_joules, units]):
        raise ValueError('invalid amortization inputs')
    difference = python_run_joules - native_run_joules
    runs = math.ceil(compile_joules / difference) if difference > 0 else None
    return dict(compilation_joules=compile_joules, saved_joules_per_run=difference,
                break_even_runs=runs, break_even_images=runs * units if runs else None,
                basis='point estimate; one application build, fresh process per run; no break-even if savings <= 0')

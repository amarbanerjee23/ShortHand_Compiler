#!/usr/bin/env python3
"""Write a claim-safe energy capability record for CI captures.

Whole-system AC energy (E0) remains unavailable on ordinary hosted CI.  The
continuous pipeline may, however, expose hardware component counters (E1) or a
validated architecture-matched model (E2/E3).  Availability is not the same as
having measured a particular benchmark window, so this file never invents
joules/task.
"""
import argparse
import datetime
import json
import pathlib
import platform

import ci_energy_evidence


def status(profile_dir=None):
    profile_dir = pathlib.Path(profile_dir) if profile_dir else pathlib.Path(__file__).with_name('calibration_profiles')
    capability = ci_energy_evidence.probe(profile_dir=profile_dir)
    highest = capability.get('highest_available_evidence_class')
    if highest == 'E1':
        source = 'rapl_available_unmeasured'
        reason = (
            'Readable CPU package energy counters are available. A separate '
            'instrumented benchmark pass is required before E1 joules/task can be reported.'
        )
    elif highest in ('E2', 'E3'):
        source = 'calibration_profile_available_unapplied'
        reason = (
            f'A hardware-matched {highest} profile is available. Joules/task remain '
            'unavailable until the required counters/workload features are captured '
            'for the benchmark window.'
        )
    else:
        source = 'unavailable'
        reason = (
            'This runner exposes neither a qualified component-energy counter nor '
            'a matching validated calibration/analytical profile. Latency and '
            'correctness remain available; joules are intentionally not fabricated.'
        )
    return {
        'schema': 'shorthand.energy.availability.v1',
        'mode': 'continuous_ci_evidence',
        'physical_energy_measured': False,
        'component_energy_measured': False,
        'evidence_class': None,
        'highest_available_evidence_class': highest,
        'energy_savings_percent': None,
        'joules_per_completed_task': None,
        'boundary': 'ci_capability_probe',
        'source': source,
        'reason': reason,
        'disallowed_substitutes': [
            'elapsed_time',
            'tdp',
            'cpu_utilization',
            'unmatched_reference_coefficients',
        ],
        'supported_evidence_classes': {
            'E0': 'externally calibrated whole-system physical measurement',
            'E1': 'hardware-reported component energy such as RAPL/NVML',
            'E2': 'architecture-matched calibrated counter model',
            'E3': 'explicit analytical operation/memory model',
        },
        'host': {
            'platform': platform.platform(),
            'hardware': capability.get('hardware'),
            'rapl_domains': capability.get('rapl_domains'),
            'perf': capability.get('perf'),
            'matching_profiles': capability.get('matching_profiles'),
        },
        'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'claim_authorized': False,
        'qualification_next_step': (
            'Run the same benchmark in a separate instrumented pass. Prefer E1 '
            'hardware counters when supported; otherwise use only a hardware-matched '
            'validated E2/E3 profile and retain its uncertainty/provenance.'
        ),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--profiles', type=pathlib.Path)
    args = parser.parse_args()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(status(args.profiles), indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()

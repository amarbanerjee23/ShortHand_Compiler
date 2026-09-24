#!/usr/bin/env python3
"""Write an explicit, claim-safe energy availability record for a capture.

The hosted experiment workflow is an execution-only campaign.  This helper
does not infer joules from elapsed time, TDP, RAPL, NVML, or a virtual-machine
power hint.  A calibrated whole-host AC meter is required for an energy claim;
the dedicated-machine campaign consumes its instrument and meter trace through
``campaign.py`` instead.
"""
import argparse
import datetime
import json
import pathlib
import platform


def status():
    powercap_paths = (
        pathlib.Path('/sys/class/powercap'),
        pathlib.Path('/sys/devices/virtual/powercap'),
    )
    return {
        'schema': 'shorthand.energy.availability.v1',
        'mode': 'execution_only',
        'physical_energy_measured': False,
        'energy_savings_percent': None,
        'joules_per_completed_task': None,
        'boundary': 'whole_host_ac',
        'source': 'unavailable',
        'reason': (
            'This hosted workflow has no calibrated, independently logging '
            'whole-host AC meter. Timing and correctness are recorded, but '
            'joules and energy savings are intentionally unavailable.'
        ),
        'disallowed_substitutes': ['elapsed_time', 'tdp', 'rapl', 'nvml', 'cpu_utilization'],
        'host': {
            'platform': platform.platform(),
            'powercap_paths_present': [str(path) for path in powercap_paths if path.exists()],
        },
        'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'qualification_next_step': (
            'Run the frozen matrix on a dedicated isolated Linux host with a '
            'calibrated whole-host AC instrument and retained meter CSV.'
        ),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(status(), indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()

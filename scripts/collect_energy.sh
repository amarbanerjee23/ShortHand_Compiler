#!/usr/bin/env bash
set -euo pipefail
echo "energy collection probe"
if [[ -r /sys/class/powercap/intel-rapl:0/energy_uj ]]; then
  echo "rapl_available=true path=/sys/class/powercap/intel-rapl:0/energy_uj"
else
  echo "rapl_available=false reason=RAPL counter unavailable or permission denied"
fi
if command -v nvidia-smi >/dev/null 2>&1; then
  echo "nvml_probe=available"
else
  echo "nvml_probe=unavailable reason=nvidia-smi not found"
fi

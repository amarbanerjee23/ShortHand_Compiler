# Frozen energy protocol: ShortHand vs C, C++, Rust and Go

This protocol decides, mechanically, whether ShortHand may claim lower measured
energy than systems languages. It adds **no** compiler, runtime or language
change; everything lives under `experiments/energy/frozen_protocol/`.

Until an aggregate result reports `energy_claim_authorized: true`, no document
may say ShortHand uses less power, saves energy, or is a "green" language in a
measured sense. Elapsed time, TDP and CPU utilisation are never substituted for
energy.

## Gates (all required)

| Gate | Enforced by | Requirement |
|---|---|---|
| Physical source | `qualify` | Linux powercap/RAPL **package** counters (sub-domains are not summed) and/or a cumulative external AC-meter command |
| Isolated, quiet host | `qualify` | not virtualised, 1-minute load ≤ 0.05 × CPUs, governor fixed to `performance` on every CPU |
| Counter integrity | `RaplSource` | single wrap handled, windows that could hold a second wrap rejected; meter must be finite and monotonic |
| Functional equivalence | `run` | every variant prints exactly the reference stdout before timing |
| Controls | `run` | randomised order per round, warm-up rounds, optional CPU pinning, idle power measured and subtracted per window |
| Statistics | `run` | ≥ 30 balanced rounds, paired candidate/baseline net-joule ratio, deterministic bootstrap 95 % CI; "lower" needs the **upper** bound < 1 |
| Two sources agree | `aggregate` | if RAPL and an AC meter are both used, medians within 25 % |
| Replication | `aggregate` | ≥ 2 distinct qualified hosts, identical frozen protocol hash, full scale (`--scale 1`) |
| Win everywhere | `aggregate` | every host and every listed baseline must report `candidate_lower_energy` |

A single host run can never authorise a claim. A claim, if authorised, covers
only the frozen workloads in `protocol.json` and does not extend to other
workloads, AI inference, or software-development workflows (compile/edit/test
loops are not measured here).

## Matrix

`protocol.json` freezes the workload (`chain`: dependent `i32` multiply-add-modulo
chain, `n` read from stdin, one checksum printed) and these variants:
ShortHand native (reference build via `short_hand compile-native`), C11 `gcc -O2`,
C++17 `g++ -O2`, Rust `rustc -O`, Go `go build`. The reference baseline is
`g++ -O2`; the candidate is ShortHand. Edit the file only by creating a new
protocol name: its SHA-256 is recorded in every run and checked on aggregation.

## Usage

```bash
# 1. On each candidate bare-metal host (performance governor, otherwise idle)
python3 experiments/energy/frozen_protocol/frozen_energy_protocol.py qualify --output qualification.json
python3 experiments/energy/frozen_protocol/frozen_energy_protocol.py run --output-dir results/host-A \
    --cpu 2 [--ac-meter-cmd "<prints cumulative joules>"]
# 2. Collect results/host-*/run.json from at least two hosts, then
python3 experiments/energy/frozen_protocol/frozen_energy_protocol.py aggregate \
    --runs results/host-A/run.json results/host-B/run.json --output aggregate.json
# Smoke/functional check on any machine (joules stay null, never claimable)
python3 experiments/energy/frozen_protocol/frozen_energy_protocol.py run --output-dir /tmp/dry \
    --allow-unqualified --scale 0.05
# Tests (no hardware needed)
python3 experiments/energy/frozen_protocol/test_frozen_energy_protocol.py
```

Run `qualify` exit codes: `0` qualified, `3` not qualified (reasons listed).

## Limits

* One workload family; results do not transfer to others.
* RAPL measures CPU package energy only, not the whole system; an AC meter is
  the independent check. Without one, a claim rests on one source.
* The Go and Rust runtimes include their own startup; every variant is measured
  as a whole process under the same conditions.

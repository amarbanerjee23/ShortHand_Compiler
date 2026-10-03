#!/usr/bin/env python3
"""Require a committed evidence-report update in every pull request."""
import argparse
import pathlib
import re
import subprocess

REPORT = "docs/latency_energy_test_and_benchmark_results.md"


def check(repo, base, head):
    def git(*args):
        return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()

    # Only immutable full SHAs may reach Git's revision parser.
    if any(not re.fullmatch(r"[0-9a-f]{40}", value) for value in (base, head)):
        raise ValueError("base and head must be full commit SHAs")
    changed = git("diff", "--name-only", base, head, "--", REPORT).splitlines()
    if REPORT not in changed:
        raise ValueError(f"every PR must update {REPORT}")
    mode = git("ls-tree", head, "--", REPORT).split()
    if not mode or mode[0] != "100644":
        raise ValueError("benchmark report must be a regular Markdown file")
    value = git("show", f"{head}:{REPORT}")
    if not value.startswith("# ShortHand testing, experimentation and benchmark results\n"):
        raise ValueError("benchmark report is missing its title")
    if f"Comparison base: `{base}`" not in value:
        raise ValueError("report must identify the current PR comparison base")
    for section in ("## Current PR evidence", "## Energy and power result", "## Reproduction"):
        if section not in value:
            raise ValueError(f"benchmark report is missing {section}")
    return f"PASS committed PR benchmark report: {REPORT}; base={base}; head={head}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    args = parser.parse_args()
    print(check(args.repo, args.base, args.head))


if __name__ == "__main__":
    main()

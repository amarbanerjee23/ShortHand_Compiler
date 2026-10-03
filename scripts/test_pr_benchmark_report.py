#!/usr/bin/env python3
import pathlib
import subprocess
import tempfile
import unittest

from check_pr_benchmark_report import REPORT, check


class ReportContractTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = pathlib.Path(self.tmp.name)
        self.git("init", "-q")
        self.git("config", "user.name", "Test")
        self.git("config", "user.email", "test@example.invalid")
        self.git("commit", "--allow-empty", "-qm", "base")
        self.base = self.git("rev-parse", "HEAD")
        self.report = self.repo / REPORT
        self.report.parent.mkdir(parents=True)

    def git(self, *args):
        return subprocess.check_output(["git", "-C", str(self.repo), *args], text=True).strip()

    def commit(self):
        self.git("add", ".")
        self.git("commit", "--allow-empty", "-qm", "head")
        return self.git("rev-parse", "HEAD")

    def valid(self):
        return ("# ShortHand testing, experimentation and benchmark results\n\n"
                f"Comparison base: `{self.base}`\n\n## Current PR evidence\n\n"
                "## Energy and power result\n\n## Reproduction\n")

    def test_accepts_committed_report(self):
        self.report.write_text(self.valid())
        self.assertIn("PASS committed", check(self.repo, self.base, self.commit()))

    def test_rejects_missing_or_worktree_only_report(self):
        head = self.commit()
        self.report.write_text(self.valid())
        with self.assertRaisesRegex(ValueError, "every PR"):
            check(self.repo, self.base, head)

    def test_rejects_stale_base(self):
        self.report.write_text(self.valid().replace(self.base, "0" * 40))
        with self.assertRaisesRegex(ValueError, "comparison base"):
            check(self.repo, self.base, self.commit())

    def test_rejects_symlink(self):
        self.report.symlink_to("/dev/null")
        with self.assertRaisesRegex(ValueError, "regular Markdown"):
            check(self.repo, self.base, self.commit())

    def test_rejects_truncated_report(self):
        self.report.write_text(self.valid().split("## Energy")[0])
        with self.assertRaisesRegex(ValueError, "missing ## Energy"):
            check(self.repo, self.base, self.commit())

    def test_rejects_non_sha_revision(self):
        with self.assertRaisesRegex(ValueError, "full commit SHAs"):
            check(self.repo, "--help", self.base)


if __name__ == "__main__":
    unittest.main()

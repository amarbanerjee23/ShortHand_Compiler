# Release claims and audit closeout

release_closeout_contract: shorthand.release.closeout.v1
release_closeout_status: implemented_no_go_pending_external_evidence
production_claim: false
publication_authorized: false

PR102 completes the currently planned implementation sequence. It does not complete the physical, independent-review or protected-release evidence. The current decision is **no-go for enterprise GA**. ShortHand remains a controlled beta in `linux-x64-cpu-v1`; no certification, certification level, accelerator production support, comparative energy superiority or universal lowest-carbon claim is authorized.

## Audit reconciliation

`docs/release_closeout_ledger.json` accounts for all 25 findings in the immutable PR93 assessment. It pins that document's SHA-256 and preserves each original ID, severity and title. Each finding records its implemented contract or explicit scope, evidence paths, owner and remaining exit. There are 6 implemented dispositions, 6 deliberately scoped dispositions and 13 findings still awaiting evidence. A scoped disposition is not proof of broader capability. The validator refuses missing/duplicate findings, missing evidence, modified historical findings and unsupported promotion of retained gaps.

TST017/TST025/TST026 and C3-ECO B/C/D/G8/S9/S12 remain blockers. The 13 audit findings additionally expose incomplete lifecycle, older-hardware, quality/safety and organizational evidence that a simple PR count would hide. There are zero further *planned implementation batches* after PR102; accepting external evidence or fixing newly discovered gaps still requires review and may require additional changes.

## Generate a candidate review

From a source checkout with Python 3:

```bash
python3 scripts/release_closeout.py --root . --revision "$(git rev-parse HEAD)" \
  --report /tmp/shorthand_release_closeout.json
```

The report binds the source revision and SHA-256/size of every consumed source artifact, derives public capabilities from production truth, reconciles both blocker matrices and lists all outstanding audit exits. It is deterministic for the same inputs. Without live GitHub reads or an executed RC report it explicitly records those missing observations. A contract report is not execution evidence.

After the inherited CI job executes PR99's real lifecycle and deployment prerequisites, pass `--rc-report /tmp/shorthand_production_rc.json`. The validator rejects a wrong commit/scope, missing or altered blocker inventory, non-boolean execution flags, mandatory skips and unsupported claims. An executed JSON receipt still requires independent review: `CI=true` or a receipt field alone cannot authenticate physical disaster recovery or organizational operations.

For live review of a clean tracked release checkout:

```bash
python3 scripts/release_closeout.py --root . --revision "$(git rev-parse HEAD)" \
  --github --repository amarbanerjee23/ShortHand_Compiler \
  --rc-report /path/to/retained-rc-report.json --report /tmp/shorthand_release_closeout.json
```

GitHub is queried directly over HTTPS for the source commit, PR102's actual merge/head, source and reviewed trees, current master lineage/protection flag and both `ci / ubuntu (push)` and `ci / ubuntu (pull_request)` contexts on the reviewed head. The release commit must be the actual merged PR commit, its tree must match the tested head, and it must remain on master lineage. An optional `GH_TOKEN`/`GITHUB_TOKEN` is used only for this fixed GitHub API origin; redirects are rejected. Unavailable APIs, malformed data or a dirty tracked checkout fail the live command. A protection flag does not replace the external review of required-reviewer, bypass, environment and credential controls.

The installed SDK includes this tool at `share/shorthand/tools/release_closeout.py`; it requires an explicit source checkout via `--root`. It is release tooling, not a compiler/runtime dependency.

## Public capability and publication policy

README, backend compatibility and public readiness each contain one generated capability block. Its exact text is derived from `docs/production_truth.tsv`; CI rejects missing, duplicate or changed blocks. The report's typed fields and these blocks are the approved current capability wording. Prose outside them still requires editorial review; this tool does not attempt to interpret arbitrary natural-language marketing claims.

Exit 0 means the candidate review contract was valid, including an explicitly blocked decision. Exit 2 means invalid/untrusted inputs or unavailable live evidence. `--require-ga` returns exit 3 while publication is unauthorized. Every v1 receipt has `publication_authorized:false`; this version cannot grant a certificate or approve GA by changing a JSON flag. The release workflow applies that requirement to non-RC versions before privileged publication. Explicit `-rc.N` versions may still exercise the existing protected candidate-publication and attestation workflow, preserving the route needed to obtain TST017 evidence.

The release workflow remains tag/protected-environment controlled; PR CI receives no additional publication permissions. Actual GA requires a separately reviewed acceptance of calibrated physical results, complete declared family/quality/safety evidence, independent organizational review and a verified protected-release exercise. This closeout preserves that no-go decision instead of treating the end of the planned PR sequence as permission to publish.

## Mandatory validation

`bash scripts/check_release_closeout.sh` exercises report construction, all retained blockers, source/claim contradictions, historical-audit integrity, document drift, path/file attacks, malformed JSON, live-state mismatches and publication rejection. It is part of production-truth qualification, inherited Make/CTest governance and installed-SDK lifecycle. Ubuntu CI also consumes its real executed RC report and retains the closeout artifact. No inherited compiler, sanitizer, live ONNX, security, Kubernetes or zero-skip gate is weakened.

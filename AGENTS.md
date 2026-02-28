## Multi-Agent Development Guide

This repository supports autonomous multi-agent collaboration with strict quality gates.

### Agent Roles

1. Planner
- Input: GitHub issue.
- Output: implementation plan, scope boundaries, and acceptance criteria.
- File scope: no direct code changes by default.

2. Implementer
- Input: plan and acceptance criteria.
- Output: code changes in `include/`, `src/`, `examples/`.
- File scope: avoid editing `tests/` unless explicitly required.

3. Tester
- Input: implementer diff and acceptance criteria.
- Output: tests and validation changes in `tests/`, CI fixes for test reliability.
- File scope: do not modify public API unless required for testability and documented.

4. SecurityChecker
- Input: implementer diff, tester handoff.
- Output: static analysis report (cppcheck, clang-tidy) with findings by severity.
- File scope: read-only for source code; writes only its handoff report file. Does not modify `include/`, `src/`, `tests/`, or `examples/`. Reports findings for the Implementer to fix.
- Outputs `AGENT_STATUS: {"status":"PASS","findings":0}` when no critical/high findings remain.

5. Reviewer
- Input: final diff and CI results, SecurityChecker report.
- Output: risk report, regression check, release notes fragment.
- File scope: docs and small fixes only.
- If blocking issues are found, control returns to the Implementer.

6. DocChecker
- Input: all prior handoffs and the full set of code changes.
- Output: documentation gap report, documentation updates applied, and a new CHANGELOG.md entry.
- File scope: `docs/`, `README.md`, `CONTRIBUTING.md`, `CHANGELOG.md`, inline code comments.
- Must verify that every public API change, new feature, and behavioral change introduced in this issue is reflected in documentation.

### Required Contract For Every Agent

Each agent must return:
- Summary of changes.
- Files changed.
- Validation commands run.
- Result of each command.
- Remaining risks or assumptions.

### Definition of Done (DoD)

A task is done only if all are true:
- Builds successfully with CMake.
- Formatting check passes.
- Tests pass.
- No unresolved critical or high severity security findings.
- Acceptance criteria from issue are satisfied.
- PR checklist is complete.

### Required Local Validation Commands

Run from repository root:

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake -P scripts/check-format.cmake
```

### Branching and Isolation

- One branch per task.
- One worktree per active agent branch.
- Never share one working directory between two active coding agents.

Use helper script:
- `scripts/setup-agent-worktrees.ps1`

### Coordination Rules

- Planner defines scope and non-goals before coding.
- The pipeline is cyclic with a configurable iteration limit (`MaxIterations`, default 3):
  1. Planner — produces the implementation plan (runs once)
  2. Implementer → Tester → SecurityChecker loop: Implementer applies changes, Tester validates, SecurityChecker performs static analysis. If Tester reports `STATUS: FAIL` or SecurityChecker reports `STATUS: FAIL`, control returns to the Implementer for fixes; this repeats until both report `STATUS: PASS` or the iteration limit is reached. If the limit is exhausted, a warning is emitted and the pipeline continues with the last known state.
  3. Reviewer → Implementer → Tester → SecurityChecker loop: Reviewer checks the result. If Reviewer reports `STATUS: NOT READY`, control returns to the Implementer (with reviewer findings), then Tester reruns, then SecurityChecker reruns, then Reviewer reruns; this repeats until `STATUS: READY` or the iteration limit is reached. If the Tester or SecurityChecker fails during a retry, the reviewer loop is broken immediately and a warning is emitted. If the iteration limit is exhausted, a warning is emitted and the pipeline continues to DocChecker.
  4. DocChecker — verifies and updates documentation and CHANGELOG.md (runs once after the review loop passes).
  5. Auto-merge — if `-PublishToGitHub` is set, the implementer branch is pushed and a pull request is created with auto-merge enabled (requires `gh` and a valid `GITHUB_TOKEN`).
- After each agent completes, its changes are committed and merged into the next agent's worktree (worktree promotion) so every agent always sees the latest code.
- If CI fails, ownership returns to the agent role that introduced the failing change.
- If acceptance criteria are ambiguous, Planner must clarify before implementation.

### Structured Handoffs

Every agent ends its output with a machine-readable JSON status line:

```
AGENT_STATUS: {"status":"PASS","findings":N}
```

- `status` is `"PASS"` / `"FAIL"` for Tester and SecurityChecker; `"READY"` / `"NOT READY"` for Reviewer.
- `findings` is the count of unresolved failures or blocking issues.
- The orchestrator (`run-multi-agent.ps1`) parses this line; free-text `STATUS: PASS` is accepted as a legacy fallback.

### Safety Rules

- No force-push to protected branches.
- No merge without green CI.
- No direct commits to `main`.

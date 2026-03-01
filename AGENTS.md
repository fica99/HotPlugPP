## Multi-Agent Development Guide

This repository supports autonomous multi-agent collaboration with strict quality gates.

### Agent Roles

1. Planner
- Input: GitHub issue.
- Output: implementation plan, scope boundaries, and acceptance criteria.
- File scope: no direct code changes by default.

2. Architect
- Input: planner handoff and issue context.
- Output: architecture handoff, implementation decomposition, constraints and integration notes.
- File scope: no direct code changes by default.

3. Implementer
- Input: planner/architect handoffs and acceptance criteria.
- Output: code changes in `include/`, `src/`, `examples/`.
- File scope: avoid editing `tests/` unless explicitly required.

4. Tester
- Input: implementer diff and acceptance criteria.
- Output: tests and validation changes in `tests/`, test reliability fixes.
- File scope: do not modify public API unless required for testability and documented.

5. SecurityChecker
- Input: implementer diff and tester handoff.
- Output: static analysis report (cppcheck, clang-tidy) with findings by severity.
- File scope: read-only for source code; writes only its handoff report file. Does not modify `include/`, `src/`, `tests/`, or `examples/`.
- Outputs `AGENT_STATUS: {"status":"PASS","findings":0}` when no critical/high findings remain.

6. Reviewer
- Input: planner/architect handoffs and implementer diff.
- Output: risk report, regression check, release notes fragment.
- File scope: docs and small fixes only.
- If blocking issues are found, control returns to the Implementer.

7. DocChecker
- Input: all prior handoffs and the full set of code changes.
- Output: documentation gap report, documentation updates applied, and a new `CHANGELOG.md` entry.
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
- `scripts/setup-agent-worktrees.ps1` (PowerShell)
- `scripts/setup-agent-worktrees.bat` (Windows wrapper)
- `scripts/setup-agent-worktrees.sh` (macOS/Linux)

### Coordination Rules

- Planner defines scope and non-goals before coding.
- The pipeline is cyclic with a configurable iteration limit (`MaxIterations`, default 2):
  1. Planner - produces the implementation plan (runs once).
  2. Architect - produces architecture and integration guidance (runs once).
  3. Implementer - applies code changes.
  4. Planner-review loop: if status is `NOT READY`, control returns to Implementer; repeat until `READY` or iteration limit.
  5. Reviewer loop: if status is `NOT READY`, control returns to Implementer; repeat until `READY` or iteration limit.
  6. SecurityChecker loop: if status is `FAIL`, control returns to Implementer; repeat until `PASS` or iteration limit.
  7. Tester loop: if status is `FAIL`, control returns to Implementer; repeat until `PASS` or iteration limit.
  8. DocChecker - verifies and updates documentation and `CHANGELOG.md` (runs once after loops).
  9. Auto-merge - if `-PublishToGitHub` is set, the implementer branch is pushed and a pull request is created with auto-merge enabled (requires `gh auth login`).
- After each agent completes, its changes are committed and promoted into the next agent's worktree.
- If validation fails, ownership returns to the agent role that introduced the failing change.
- If acceptance criteria are ambiguous, Planner must clarify before implementation.

### Structured Handoffs

Every agent ends its output with a machine-readable JSON status line:

```
AGENT_STATUS: {"status":"PASS","findings":N}
```

- `status` is `"PASS"` / `"FAIL"` for Tester and SecurityChecker; `"READY"` / `"NOT READY"` for Planner-review and Reviewer.
- `findings` is the count of unresolved failures or blocking issues.
- The orchestrator (`run-multi-agent.ps1`) parses this line; free-text `STATUS: PASS` is accepted as a legacy fallback.

### Safety Rules

- No force-push to protected branches.
- No merge without successful validation.
- No direct commits to `main`.

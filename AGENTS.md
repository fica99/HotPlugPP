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

4. Reviewer
- Input: final diff and CI results.
- Output: risk report, regression check, release notes fragment.
- File scope: docs and small fixes only.
- If blocking issues are found, control returns to the Implementer.

5. DocChecker
- Input: all prior handoffs and the full set of code changes.
- Output: documentation gap report and any documentation updates applied.
- File scope: `docs/`, `README.md`, `CONTRIBUTING.md`, inline code comments.
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
  2. Implementer → Tester loop: Implementer applies changes, Tester validates. If Tester reports `STATUS: FAIL`, control returns to the Implementer for fixes; this repeats until `STATUS: PASS` or the iteration limit is reached. If the limit is exhausted, a warning is emitted and the pipeline continues with the last known state.
  3. Reviewer → Implementer → Tester loop: Reviewer checks the result. If Reviewer reports `STATUS: NOT READY`, control returns to the Implementer (with reviewer findings), then Tester reruns, then Reviewer reruns; this repeats until `STATUS: READY` or the iteration limit is reached. If the Tester fails during a retry, the reviewer loop is broken immediately and a warning is emitted. If the iteration limit is exhausted, a warning is emitted and the pipeline continues to DocChecker.
  4. DocChecker — verifies and updates documentation (runs once after the review loop passes).
- If CI fails, ownership returns to the agent role that introduced the failing change.
- If acceptance criteria are ambiguous, Planner must clarify before implementation.

### Safety Rules

- No force-push to protected branches.
- No merge without green CI.
- No direct commits to `main`.

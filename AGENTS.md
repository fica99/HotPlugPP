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

5. Fixer
- Input: reviewer findings.
- Output: code changes addressing reviewer findings in `include/`, `src/`, `tests/`, `examples/`.
- File scope: any file flagged by the reviewer.
- Must re-run build/test/format checks after applying fixes.

6. DocChecker
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
- Agent execution order:
  1. Implementer — applies code changes
  2. Tester — adds/fixes tests
  3. Reviewer — provides findings
  4. Fixer — addresses reviewer findings
  5. DocChecker — verifies and updates documentation
- If CI fails, ownership returns to the agent role that introduced the failing change.
- If acceptance criteria are ambiguous, Planner must clarify before implementation.

### Safety Rules

- No force-push to protected branches.
- No merge without green CI.
- No direct commits to `main`.

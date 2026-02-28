# Multi-Agent Workflow

This document defines how to run autonomous development safely in this repository.

## 1. Create a task with strict acceptance criteria

Create an issue using:
- `Feature request` template for new capabilities.
- `Bug report` template for defects.

Every task must have measurable acceptance criteria.

## 2. Isolate agent work

Create dedicated worktrees so agents do not conflict:

```powershell
pwsh ./scripts/setup-agent-worktrees.ps1
```

This creates separate branches and directories for:
- planner
- implementer
- tester
- reviewer

## 3. Execute role order

1. Planner: define scope, non-goals, acceptance checks.
2. Implementer: deliver code changes.
3. Tester: add/adjust tests and validate behavior.
4. Reviewer: assess regression, compatibility, and merge readiness.

## 4. Enforce DoD locally

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake -P scripts/check-format.cmake
```

## 5. Enforce DoD in CI

CI must pass before merge:
- build and test matrix
- format check
- filename check

## 6. Merge policy

- Use pull requests only.
- Require green CI checks.
- Require completed PR checklist.
- Do not merge directly into `main`.

## 7. Branch protection (GitHub settings)

Configure branch protection for `main`:
- Require a pull request before merging.
- Require status checks to pass before merging.
- Require branches to be up to date before merging.
- Restrict who can push to matching branches (optional but recommended).

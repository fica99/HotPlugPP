# Multi-Agent Workflow

This document defines how to run autonomous development safely in this repository.

## 1. Create a task with strict acceptance criteria

Create an issue using:
- `Feature request` template for new capabilities.
- `Bug report` template for defects.

Every task must have measurable acceptance criteria.

Add the `agent-run` label to the issue to trigger the pipeline automatically via GitHub Actions, or run it manually as described below.

## 2. Isolate agent work

Create dedicated worktrees so agents do not conflict:

```powershell
pwsh ./scripts/setup-agent-worktrees.ps1
```

This creates separate branches and directories for:
- planner
- implementer
- tester
- security-checker
- reviewer
- doc-checker

## 2.1 Optional: Run all roles automatically

Use the orchestrator script to run the full pipeline end-to-end:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run-multi-agent.ps1 -IssueNumber 18
```

Optional flags:
- `-IssueContextFile <path>` when GitHub CLI is unavailable.
- `-PublishToGitHub` to post role outputs as issue comments and create a PR with auto-merge enabled (requires `gh auth login`).
- `-Model <name>` to force a Codex model.
- `-DryRun` to validate pipeline files/prompts without calling Codex.
- `-MaxIterations <n>` to cap feedback cycles (default: 3).

## 2.2 Automatic trigger via GitHub Actions

Label any issue with `agent-run` to trigger `.github/workflows/multi-agent.yml` automatically. The workflow:
1. Installs the OpenAI Codex CLI.
2. Creates agent worktrees.
3. Runs the full pipeline with `-PublishToGitHub`.
4. Uploads all handoff files as workflow artifacts.

Required repository secrets: `OPENAI_API_KEY`.

## 3. Pipeline

```
Planner → [Implementer → Tester → SecurityChecker]* → [Reviewer → Implementer → Tester → SecurityChecker]* → DocChecker → (auto-merge PR)
```

`*` repeats until all agents pass or `MaxIterations` is reached.

1. **Planner**: define scope, non-goals, acceptance checks.
2. **Implementer**: deliver code changes.
3. **Tester**: add/adjust tests and validate behavior; reports `AGENT_STATUS: {"status":"PASS"}`.
4. **SecurityChecker**: run cppcheck/clang-tidy; reports `AGENT_STATUS: {"status":"PASS"}` when no critical/high findings remain.
5. **Reviewer**: assess regression, compatibility, and merge readiness; reports `AGENT_STATUS: {"status":"READY"}`.
6. **DocChecker**: verify and update docs, README, CONTRIBUTING, and CHANGELOG.
7. **Auto-merge**: push implementer branch and open a PR with auto-merge (when `-PublishToGitHub`).

### Feedback loops

- **Tester FAIL or SecurityChecker FAIL** → returns to Implementer with failure details.
- **Reviewer NOT READY** → returns to Implementer, reruns Tester and SecurityChecker, then reruns Reviewer.
- Each agent's changes are merged into the next agent's worktree (worktree promotion) before that agent runs.

### Structured handoffs

Every agent ends its output with:
```
AGENT_STATUS: {"status":"PASS","findings":N}
```
The orchestrator parses this JSON line to determine loop continuation.

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

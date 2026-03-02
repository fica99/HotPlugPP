Issue #19.

Read:
- Issue context: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-19-context.md
- Planner handoff: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-19-planner.md
- Architect handoff: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-19-architect.md
- Implementer handoff: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-19-implementer.md

Task:
- Add/update tests for acceptance criteria.
- Run build/test/format checks relevant to the change.
- If failures occur, fix test or code issues in this worktree.
- At end, report:
  - Files changed
  - Command results (pass/fail)
  - Residual risks
- End the output with exactly one JSON line:
  AGENT_STATUS: {"status":"PASS","findings":N}
  or
  AGENT_STATUS: {"status":"FAIL","findings":N}
  where N is the count of failing tests or build errors.

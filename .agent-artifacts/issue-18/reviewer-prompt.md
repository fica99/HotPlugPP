Issue #18.

Read:
- Issue context: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-18-context.md
- Planner handoff: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-18-planner.md
- Implementer handoff: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-18-implementer.md
- Tester handoff: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-18-tester.md
- SecurityChecker handoff: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-18-security-checker.md

Task:
- Perform review with focus on bugs, regressions, API compatibility, and testing gaps.
- Return findings ordered by severity.
- End the output with exactly one JSON line:
  AGENT_STATUS: {"status":"READY","findings":N}
  or
  AGENT_STATUS: {"status":"NOT READY","findings":N}
  where N is the count of blocking findings.

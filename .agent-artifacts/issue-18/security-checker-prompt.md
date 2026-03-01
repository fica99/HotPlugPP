Issue #18.

Read:
- Issue context: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-18-context.md
- Planner handoff: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-18-planner.md
- Implementer handoff: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-18-implementer.md
- Tester handoff: C:\Users\ficac\Desktop\projects\my\HotPlugPP-handoff\issue-18-tester.md

Task:
- Run static analysis tools (cppcheck, clang-tidy if available) on changed source files.
- Identify security vulnerabilities, memory safety issues, and undefined behaviour.
- Document findings with severity (critical/high/medium/low) and file/line references.
- Do NOT modify source files; report findings only so the Implementer can apply fixes.
- At end, report:
  - Tools run and commands used
  - Findings by severity (critical/high/medium/low)
  - Accepted risks with justifications
- End the output with exactly one JSON line:
  AGENT_STATUS: {"status":"PASS","findings":N}
  or
  AGENT_STATUS: {"status":"FAIL","findings":N}
  where N is the count of unresolved critical or high severity findings.

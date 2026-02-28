## Summary

Describe what changed and why.

## Acceptance Criteria Mapping

- [ ] Criterion 1 satisfied
- [ ] Criterion 2 satisfied
- [ ] Criterion 3 satisfied (if applicable)

## Agent Handoff

- Planner notes:
- Implementer notes:
- Tester notes:
- Reviewer notes:

## Validation

- [ ] `cmake -S . -B build`
- [ ] `cmake --build build --config Release --parallel`
- [ ] `ctest --test-dir build -C Release --output-on-failure`
- [ ] `cmake -P scripts/check-format.cmake`

## Risk Review

- [ ] API compatibility checked
- [ ] Runtime hot-reload behavior checked
- [ ] Cross-platform impact considered (Windows/Linux/macOS)

## Documentation

- [ ] README/Wiki updated if behavior or API changed

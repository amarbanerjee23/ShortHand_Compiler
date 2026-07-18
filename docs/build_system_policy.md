# ShortHand Build System Policy

CMake is the production build system for ShortHand.

`Compiler_new_ws/Short_Hand/src/Makefile` is retained as a temporary CI compatibility harness because the current workflow still uses Makefile targets for:

- legacy compiler smoke tests,
- sanitizer tests,
- focused AI runtime tests,
- green AI tool checks.

The Makefile must not drift from CMake. Any new production source file must be added to both CMake and the Makefile until the Makefile-backed CI targets have equivalent CTest coverage.

## Decommission rule

The Makefile may be removed only after all of these are true:

1. `make -C Compiler_new_ws/Short_Hand/src test` has a CTest equivalent.
2. `make -C Compiler_new_ws/Short_Hand/src sanitize` has a CMake/CTest sanitizer equivalent.
3. AI runtime and evidence tests are represented in CTest.
4. CI no longer calls any Makefile target.

Until then, deleting the Makefile would weaken the current validation pipeline.

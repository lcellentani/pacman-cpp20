# Troubleshooting & Known Limitations

Root-caused postmortems for known, currently-unfixable limitations in this project's
toolchain and Claude Code setup. Consult this file when you hit one of these symptoms —
they don't need to be loaded every session.

## VS Code debugger (`cppvsdbg`) can't expand struct/class members on the clang-ninja-debug preset

Breakpoints and scalar locals work; compound variables (`GhostDebugState`, `PacmanDebugState`,
`MapCoord`, `AABB`, etc.) show as empty/unexpandable.

Verified with `llvm-pdbutil dump -types` on the linked PDB: Clang emits these object files
with `-debug-info-kind=constructor`, a "homing" heuristic that only emits a type's full layout
in the one TU that defines its constructor. These are plain aggregates with no constructor, so
no TU is ever homed and every object file carries only a forward declaration (`sizeof 0`).

`-fstandalone-debug` (Clang's documented escape hatch from homing) does **not** fix this for
types defined in a C++20 module interface unit — confirmed by rebuilding and re-inspecting the
PDB; the forward ref persists even in TUs that construct the type (e.g. `ghost.cpp`).

Giving the struct an explicit constructor is not a workaround either — it breaks
aggregate-initialization at every call site (e.g. `ghost.cpp:252`'s 10-element brace-init of
`GhostDebugState`).

This appears to be a real, currently-incomplete area of Clang's C++20 Standard Modules
debug-info support on the CodeView/PDB backend (more mature on DWARF/Linux). No local fix
found; not re-investigated unless Clang/LLVM changes upstream.

## `guard-module-writes.sh` hook is not active

The hook (`.claude/hooks/guard-module-writes.sh`) is designed to deny writes to `.ixx` files
outside `src/`, enforcing the module layout constraint. It's wired to a `PreToolUse` event
with `permissionDecision: "deny"`, but this is blocked by Claude Code bug
[#37210](https://github.com/anthropics/claude-code/issues/37210) (filed March 2026, still
open): `permissionDecision: "deny"` is honoured for `Bash` tool calls but **silently ignored
for `Edit` and `Write`** — the file is modified anyway.

The hook script exists and is correct; re-enable it in `.claude/settings.json` once the bug is
resolved.

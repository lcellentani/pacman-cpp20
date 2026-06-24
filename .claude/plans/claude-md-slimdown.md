# Slim down CLAUDE.md by distributing content into docs/ and .claude/

## Context

`CLAUDE.md` is read by Claude Code at the start of every session. It has grown past its
intended purpose — alongside the small, must-know-every-session facts (current phase, open
items, build commands, conventions table), it now also carries large reference blocks that
were useful once but don't need to occupy every session's context: a "Phase 3 Context"
section written as one-time codebase archaeology, a long root-caused postmortem on a VS
Code/clang debugger limitation, a long root-caused postmortem on a blocked PreToolUse hook
bug, and a full module graph + per-module architecture description that duplicates (and is
more current than) a stale copy already living in `docs/PROJECT.md`.

The goal is a lean CLAUDE.md: short orientation + the few things that must always be loaded,
with detail pushed out to dedicated files using the pattern the project already applies
successfully elsewhere (Coding Conventions → `docs/CODING_STANDARDS.md`, Commit Tags →
`docs/WORKING_AGREEMENT.md`, Custom Commands → `.claude/commands/*.md`).

Two decisions were made with the user before finalizing this plan:
- `docs/PROJECT.md` — not CLAUDE.md — becomes the single source of truth for the current
  module graph and per-module architecture. Its existing "Module graph" and "Project
  structure" sections are Phase-1-era stale (3 modules, vs. the ~15-module structure that
  exists today) and will be replaced with current content. CLAUDE.md keeps only a one-line
  pointer.
- `docs/ghost-ai-spec_dup.md` keeps its name. It is **not** touched by this plan — the user
  has a newer, not-yet-committed version of this file staged on another machine, so renaming
  or editing it now would conflict with that pending work.

## Target layout

### New files

- **`docs/PHASE3_NOTES.md`** — the Phase 3 codebase-archaeology reference, moved verbatim in
  spirit from CLAUDE.md's current "Phase 3 Context" section: verified module graph (as it
  stood entering ghost-AI work), verified SDL boundary description, the Concept system
  table, the debug state pattern description. Also absorbs the detailed implementation
  narrative currently bloating CLAUDE.md's "Current Status" (specific awaitable names,
  `arcade_ahead` overflow bug, `cardinal_dirs()` tie-break order, house-release specifics) —
  this is implementation-level detail, not a always-load-it fact.
- **`docs/TROUBLESHOOTING.md`** — consolidates the two long known-limitation postmortems that
  currently bloat CLAUDE.md's "Build" and "Hooks" sections:
  - VS Code `cppvsdbg` can't expand struct members on the clang-ninja-debug preset (the
    `llvm-pdbutil`/homing root-cause writeup).
  - `guard-module-writes.sh` is inactive because of upstream Claude Code bug #37210
    (`permissionDecision: "deny"` silently ignored for Edit/Write).
  Both are "consult when relevant" postmortems, not session-start material.

### Updated files

- **`docs/PROJECT.md`**:
  - Replace the stale "Module graph" section (3-module Phase-1 sketch) with the current
    verified module graph (the same diagram presently in CLAUDE.md's "Module Graph" /
    "Phase 3 Context" sections).
  - Add per-module architecture one-liners (currently in CLAUDE.md's "Architecture"
    section) — PROJECT.md gains a real "Architecture" subsection, since today it only has
    the stale diagram with no per-module description.
  - Update the stale "Project structure" file tree to reflect the current `src/engine` +
    `src/game` layout and current `docs/` contents.
  - This makes PROJECT.md the authoritative, current source of truth for project structure.

- **`CLAUDE.md`** — slimmed to this table of contents:
  1. **Before starting any session** — unchanged (WORKING_AGREEMENT.md pointer)
  2. **Project Overview** — unchanged, one paragraph
  3. **Current Status** — trimmed back to the originally-intended 3–5 lines, present tense;
     add a pointer to `docs/PHASE3_NOTES.md` for implementation detail
  4. **Open Items** — unchanged
  5. **Build** — keep toolchain shell requirement, cmake commands, dependency versions only;
     replace the clang debugger blockquote with a one-line pointer to
     `docs/TROUBLESHOOTING.md`
  6. **Architecture** — collapsed to a brief one-liner + pointer to `docs/PROJECT.md`'s
     Module graph & Architecture sections (no diagram duplicated in CLAUDE.md anymore)
  7. **Coding Conventions** — unchanged (already a good condensed-pointer pattern)
  8. **MCP Servers** — condensed to one line (drop endpoint/scope detail not needed every
     session)
  9. **Plans** — unchanged
  10. **Custom Commands** — unchanged
  11. **Hooks** — keep the table (it's short and tells Claude what fires on writes); replace
      the bug-root-cause blockquote with a one-line pointer to `docs/TROUBLESHOOTING.md`
  12. **Development Phases** — unchanged short status list (full rationale per phase already
      lives in PROJECT.md's "Phased roadmap")
  13. **Commit Tags** — unchanged (already a good condensed-pointer pattern)
  - Remove the standalone "Phase 3 Context" and "Module Graph" headers entirely — their
    content has moved to `docs/PHASE3_NOTES.md` and `docs/PROJECT.md` respectively.

### Untouched

`docs/WORKING_AGREEMENT.md`, `docs/CODING_STANDARDS.md`, `docs/ghost-ai-spec_dup.md`,
`docs/devlog/*`, `.claude/agents/*`, `.claude/commands/*`, `.claude/hooks/*`,
`.claude/skills/*`, `.claude/settings.json` — these already hold their content correctly;
CLAUDE.md's existing pointers to them are preserved as-is.

## Execution steps

1. Create `docs/PHASE3_NOTES.md` with the archaeology content + detailed current-state
   narrative, lightly re-headed (not a verbatim dump — adjust headers so it reads as a
   standalone reference doc rather than a CLAUDE.md fragment).
2. Create `docs/TROUBLESHOOTING.md` with the two postmortems, each under its own heading.
3. Edit `docs/PROJECT.md`: replace "Module graph" diagram, add "Architecture" subsection,
   update "Project structure" tree.
4. Edit `CLAUDE.md` per the table of contents above — remove moved content, add the short
   pointers, trim "Current Status" back to 3–5 lines.
5. Per project convention (CLAUDE.md: "Plans are saved to `.claude/plans/` inside the
   project, not the user-level path"), once this plan exits plan mode, copy this plan file
   into `D:\DEV\pacman-cpp20\.claude\plans\claude-md-slimdown.md` so it is version-controlled
   alongside the code it describes.

## Verification

- Read through the new `CLAUDE.md` top to bottom and confirm every section is either
  unchanged-and-short or a pointer — no orphaned headers, no broken cross-references.
  Adjust ToC if some moved content is more relevant in a different doc.
- Grep the repo for any other reference to the moved section headers (e.g. "Phase 3
  Context") to make sure nothing else links to content that moved.
- Confirm `docs/PROJECT.md`'s new module graph and architecture content matches what's
  actually in the codebase (cross-check against the `.ixx` files), since the old content
  going stale once is the exact failure mode this plan is fixing.
- Re-read `docs/PHASE3_NOTES.md` and `docs/TROUBLESHOOTING.md` standalone (without
  CLAUDE.md's surrounding context) to confirm they make sense as independent reference
  documents.

# Working Agreement

This document is loaded at the start of every Claude Code session. It carries the
context that makes the collaboration principled rather than generic: who is driving,
how decisions get made, what has already been decided and why, and what should never
be touched without an explicit conversation first.

`CLAUDE.md` tells Claude Code *how to build*. This document tells it *how to work*.

---

## Who this is for

**Ludovico Cellentani** — 52, Italian, seasoned game developer since 1998. Currently
Technical Director for Frostbite at Electronic Arts. Previously at King, where he was
part of the team that built the proprietary engine powering the Candy Crush franchise.

This project exists for three reasons, in order of depth:

1. **Reconnecting with the craft.** Years of TD work means architecture reviews,
   roadmaps, and people decisions. This project is about reclaiming direct contact with
   the thing that started it all.
2. **Proving sharpness.** The honest question every engineer in leadership asks: *am I
   still sharp at the low level?* The answer is found by doing, not assuming.
3. **Intellectual curiosity.** C++20 is a meaningful evolution of the language.
   Coroutines, Modules, and Concepts change how you think about program structure — not
   just the syntax.

This context matters because it shapes what "help" means here. The goal is not to
produce a finished game as fast as possible. The goal is for Ludo to build it, and to
know he built it.

---

## How we work — the delegation framework

Every commit is tagged with its delegation tier. Before any implementation work, the
tier is chosen consciously. The tiers are not about code quality — they are about
cognitive ownership.

| Tier | Name | What it means in practice |
|---|---|---|
| **T1** | Author drives, AI silent | Ludo writes the code independently. AI is not consulted until a review is explicitly requested. Applies to all C++20 feature implementations, all architectural decisions, ghost AI logic, entity system design. |
| **T2** | Author drives, AI navigates | Ludo writes the code, but may ask questions mid-implementation. "Does this coroutine structure make sense?" "Am I misusing Concepts here?" AI responds to what has been built — it does not build in place of Ludo. |
| **T3** | AI drafts, author owns | AI produces a starting point. Ludo reads every line, understands it, modifies what doesn't feel right, and commits it only when it genuinely belongs to him. Applies to CMake wiring, SDL2 boilerplate, class skeletons, doc formatting. |
| **T4** | Full delegation | AI handles it, Ludo moves on. Applies to cryptic compiler errors with no learning value, documentation formatting, repetitive patterns already understood from a prior instance. |

**The practical test:** if the task requires thinking in C++20, it's T1 or T2. If it
requires remembering an API, it's T3 or T4.

### What AI navigation looks like in practice (T2)

When Ludo signals T2 work — "I want to drive this, navigate for me" — the correct
response is:
- Ask clarifying questions about intent before proposing solutions
- Point out risks and tradeoffs in what has been built, not what should have been built
- Explain *why* something is a problem, not just that it is
- Stop before writing code unless explicitly asked

Do not volunteer implementations. Do not complete partial code unprompted. Do not
reframe the problem as an invitation to take over.

---

## What you should never do

These are standing constraints. They apply in every session and require an explicit
conversation to revisit — not a request in passing.

**Never suggest virtual dispatch or base class inheritance in the entity system.**
The design decision to use Concepts instead of vtables is not a preference — it is the
architectural spine of Phase 2. If a design problem seems to call for a base class, the
correct response is to raise the tension, not resolve it with inheritance.

**Never reintroduce dropped abstractions without saying so.**
The following were deliberately removed and must not reappear silently:
- `drawableObjects_` vector-of-variants — removed in favour of direct `draw()` calls in
  `Stage::render()`. The variant/visitor pattern is deferred until Phase 3 when dynamic
  entities (ghosts) make it necessary.
- `WallQuery` / `WorldQuery` / `PortalQuery` concepts — deferred until a second concrete
  world query source exists. They earn their place the moment two types need to be
  treated interchangeably. That moment has not arrived.

**Never add complexity to the entity system without being asked.**
The pattern for this project is: earn every abstraction. If an idea feels clever, that
is a reason to pause, not to proceed.

**Never modify `docs/PROJECT.md`.**
That document is Ludo's — written by him, for him and for the repo. It is not
operational context for Claude Code.

**Never assume phase context from the code alone.**
The code tells you what is implemented. This document tells you why. Read both.

---

## Architectural decisions and their rationale

This section records the *why* behind non-obvious design choices. When a decision is
here, it means it was made deliberately and the reasoning was documented. Revisiting it
requires a conversation, not a refactor.

### `Stage` owns the game logic; `Map` is a pure data store

`Map` is a 28×31 tile grid with query and mutation methods. It has no game semantics —
it does not know about scores, Pac-Man, or ghosts. Game logic (pellet collection, score
increment, entity orchestration) lives entirely in `Stage`. This boundary is
intentional and should be maintained as Phase 3 entities are added.

### `Pacman` uses an integer tile model (`col_`, `row_`, `offset_`)

Float position models for tile-based movement carry implicit fragility — a fractional
drift causes tile-center detection to fail silently. The integer model eliminates this:
`offset_ == 0` is an exact comparison, no tolerance required. Pixel position for
rendering is always derived on demand from the three integers.

Key invariants of this model:
- `offset_` is gated on having an active direction — it does not increment when
  `current_dir_ == {0,0}`
- Opposite-direction reversal requires both mirroring `offset_` *and* advancing the
  tile coordinate simultaneously; doing only one produces a visible position jump
- `is_opposite` guards against the zero direction — no direction is not opposite to
  anything

### `Pacman` receives `Map` via `reset(const Map* map)`, not construction

Separates resource acquisition from game state initialisation. `Pacman` can be
constructed without a `Map`; it cannot play without one. The dependency is a raw
`const Map*` (not a reference) because a reference cannot be rebound after
construction — the pointer allows `reset()` to be called again if the game restarts.
An `assert(map_)` in `can_move` enforces the precondition at debug time.

This scales cleanly to Phase 3: ghost constructors take no arguments, map dependency
is threaded once through `Stage::reset()`.

### Wall collision is owned by `Pacman::update`, not a separate step

An earlier design proposed a `resolveWorldCollisions(const Map& map)` method as an
explicit pre-update step, with `Stage` driving the sequence. That design was not
implemented. The actual implementation encapsulates collision entirely inside
`Pacman::update(float dt)`: the `can_move(col_, row_, dir)` helper queries `map_`
directly via the stored pointer, and `update` stops movement before entering a wall
tile. `Stage::update` drives a simpler sequence:

```cpp
pacman_entity_.handleInput(input);
pacman_entity_.set_speed(config_.pacman_speed);
pacman_entity_.update(dt);
```

The consequence is that `Map` is a stored dependency on `Pacman` (via `reset`), not a
per-frame parameter. The tradeoff was accepted: the encapsulation is cleaner for a
single entity, and the design can be revisited if ghost entities require different
collision behaviour.

### `Updatable` concept is deliberately minimal (`update(float dt)` only)

An earlier design attempted to express `Pacman`'s dependency on `Map` through the
`Updatable` concept — making `update` a template parameterised on a world type. This
caused cascading problems: template definitions had to live in `.ixx`, concept
constraints became unwieldy, and the concept was doing two jobs at once (defining a
contract *and* encoding a dependency). Collision moved inside `update`, the world
dependency became a stored pointer, and `Updatable` stayed minimal. Keep it that way.

### `DebugView` submits to ImGui directly; it has no dependency on `Renderer`

`DebugView::draw` populates ImGui draw lists. The flush to screen happens in
`Renderer::imgui_render()`, which `Stage` calls explicitly after all debug content is
submitted. Boundary: `DebugView` owns the *what*, `Renderer` owns the *when*.

### `PacmanDebugState` lives in `game.types`, not `engine.types`

`engine.types` holds geometric primitives with no game semantics (`AABB`, `Vec2`).
`PacmanDebugState` is game-specific vocabulary. The layer boundary must not be
violated: engine modules have no knowledge of game concepts.

---

## Current phase entry conditions

### Phase 3 — Coroutine ghost AI

**Goal:** replace placeholder ghost logic with coroutine-based state machines. Each
ghost runs as an independent coroutine. `scatter`, `chase`, `frightened`, and `dead`
states are `co_await` suspension points. Correct personalities for all four ghosts.

**Entry conditions — all met:**
- [x] Real delta time in place (`std::chrono::steady_clock` in `main.cpp`)
- [x] Tile-aligned movement working correctly
- [x] Pellet collection wired up
- [x] `game.config` in place for runtime-tweakable AI parameters

**Known open items entering Phase 3:**
- `InputPoller` refactor deferred — edge detection currently lives in `Stage` via
  `prev_debug_key_` bool. Fine for now; revisit when more edge-triggered actions appear.
  Marked in code: `// TODO(phase3): refactor poll_input into stateful InputPoller`
- `WallQuery`/`WorldQuery` concepts on hold — reintroduce when ghost collision
  resolution is wired up and a second world query source genuinely exists
- Ghost entities will each need a debug state struct following the `PacmanDebugState`
  pattern; design this before implementing ghost rendering

**Phase 3 is T1/T2 by default.** The coroutine scheduler design and ghost AI logic are
the reason this project exists. They are not delegated.

---

## Devlog expectations

Every session ends with a devlog entry committed to `docs/devlog/`. The template lives
at `docs/devlog/session-note-template.md`. Key sections:

- **What I worked on** — scope and outcome
- **What I learned** — concrete learnings with code examples where relevant
- **What surprised me** — honest reflection, including over-reliance on AI when it
  occurred
- **Delegation observations** — which tiers applied and why; self-corrections if the
  initial tier classification was off
- **Blockers / open questions** — anything unresolved
- **Next session** — concrete starting point

Devlog entries are Ludo's voice, not a summary generated by AI. AI may draft a
skeleton (T3), but the content — especially "what surprised me" and "delegation
observations" — must be authored by Ludo.

---

## A note on feedback style

Ludo values directness. No filler, no preamble, no "great question." Get to the point.

When raising a design concern, lead with the concern and the reasoning — not with
reassurance. If something is wrong, say it is wrong and explain why. If there is a
better approach, say so and defend it. Calibrate technical depth to someone with 25+
years of professional game development experience; do not over-explain fundamentals.

Ludo is not a native English speaker and welcomes corrections and suggestions that
improve precision or clarity in written communication.

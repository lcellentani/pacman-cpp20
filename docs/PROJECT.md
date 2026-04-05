# Pac-Man — A C++20 Learning Project

> *"On week 16, you open the repo, play the game, read the code — and feel like yourself again."*

---

## What this project is

A fully playable Pac-Man clone built with C++ and SDL2, where every major architectural decision is driven by a specific C++20 feature. Equal weight on game quality and language mastery.

This is not a prototype or a throwaway experiment. The code should be something a senior engineer is proud to show — clean module boundaries, well-reasoned design choices, and a game that actually feels good to play.

---

## Why this project exists

Three motivations, in order of depth:

1. **Reconnecting with the craft.** Years of Technical Director work means architecture reviews, roadmaps, and people decisions. This project is about reclaiming direct contact with the thing that started it all — writing code, building a game, solving concrete problems.

2. **Proving sharpness.** The honest question every engineer in leadership asks: *am I still sharp at the low level?* The answer is found by doing, not assuming.

3. **Intellectual curiosity.** C++20 is a meaningful evolution of the language. Coroutines, Modules, and Concepts are not syntactic sugar — they change how you think about program structure. This project is the playground for that exploration.

---

## What success looks like

| Dimension | Definition |
|---|---|
| **Technical** | Each C++20 feature is used where it is the best tool. Every architectural choice can be defended in a code review. No feature is forced. |
| **Game** | Fully playable — classic maze, four ghosts with correct AI personalities, pellets, scoring, lives. Show-worthy to any colleague. |
| **Personal** | The code reads like it was written by someone who loves this. Because it was. |

---

## Toolchain

| Component | Choice |
|---|---|
| Language | C++20 |
| Compiler | MSVC 19.50+ (`cl.exe`) |
| IDE | Visual Studio 2026 (version 18.x) |
| Build system | CMake 4.x with Ninja generator |
| Rendering | SDL2 (via CMake `FetchContent`) |
| Platform | Windows x64 |

**Verify your setup:**
```bash
# From the x64 Developer Command Prompt for VS 2026
cl.exe
# Expected: Microsoft (R) C/C++ Optimizing Compiler Version 19.50.x for x64

cmake --version
# Expected: cmake version 4.x.x
```

**Build commands:**
```bash
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
```

---

## C++20 features — the three pillars

Each feature maps to a specific game system. The mapping is not arbitrary — in each case, the feature is the best tool for that problem.

### 1. Coroutines → Ghost AI + game loop

Ghost AI in Pac-Man is a state machine: `scatter → chase → frightened → dead`. The classic C implementation is a `switch` with timers. With coroutines, each ghost *is* a coroutine — its behavioral sequence becomes control flow, and `co_await` replaces timer polling. The result reads like a screenplay, not a state table.

The game loop itself becomes a cooperative task scheduler built on coroutine tasks.

**Why coroutines fit here:** state machines with timing are exactly the problem coroutines were designed to solve cleanly.

### 2. Modules → Project architecture

Hard compile-time boundaries between `engine.game`, `engine.renderer`, and `engine.input`. No leaking includes, no implicit transitive dependencies. The module graph *is* the architecture diagram.

**Why modules fit here:** a learning project is the ideal place to internalize module discipline, because the codebase is small enough to see the whole picture but structured enough to feel the benefits.

### 3. Concepts → Entity system

`Drawable`, `Updatable`, `Collidable` as compile-time constraints. Generic systems — rendering loop, update loop, collision detection — operate on anything that satisfies the relevant concept. No virtual dispatch, no vtables, no base class inheritance chain.

**Why concepts fit here:** this maps directly to the kind of policy-based design found in production ECS architectures. It also changes how you *think* about generic code, not just the syntax.

---

## Phased roadmap

Time budget: ~2–5 hours per week. Total duration: ~16 weeks.

### Phase 1 — Foundation (weeks 1–3)
**Goal:** module graph, SDL2 window, tile map rendering, Pac-Man moving with arrow keys.

- Set up CMake with `FILE_SET CXX_MODULES`
- Define three modules: `engine.renderer`, `engine.input`, `engine.game`
- Render a simplified but recognisable Pac-Man tile map
- Pac-Man moves, wall collision works
- No game logic yet — this is infrastructure

**C++20 focus:** Modules

**Success criterion:** a window opens, Pac-Man moves, walls stop him. The module graph compiles cleanly with no circular dependencies.

---

### Phase 2 — Concepts-driven entity system (weeks 4–7)
**Goal:** define the entity model using C++20 Concepts before any AI logic is written.

- Define `Drawable`, `Updatable`, `Collidable` concepts
- Implement Pac-Man, ghosts (placeholder AI), pellets, and walls as constrained types
- Wire the update and render loops to operate on concept-constrained types
- No virtual dispatch anywhere in the entity system

**C++20 focus:** Concepts

**Success criterion:** adding a new entity type requires satisfying a concept, not inheriting from a base class. The compiler error messages when a type fails a concept are readable and useful.

---

### Phase 3 — Coroutine ghost AI + game loop (weeks 8–13)
**Goal:** replace placeholder ghost AI with coroutine-based state machines.

- Implement a minimal coroutine task scheduler for the game loop
- Each of the four ghosts runs as an independent coroutine
- `scatter`, `chase`, `frightened`, and `dead` states are `co_await` suspension points
- Correct ghost personalities: Blinky (direct chase), Pinky (ambush), Inky (flanking), Clyde (random)
- Frightened mode triggers on power pellet, ghosts resume correct state on expiry

**C++20 focus:** Coroutines

**Success criterion:** ghost behavior is implemented as readable coroutine code. The state transitions are control flow, not flag comparisons. The game is genuinely hard to beat.

---

### Phase 4 — Polish (weeks 14–16)
**Goal:** the game feels complete and shippable.

- Lives system (three lives, game over screen)
- Level progression (speed increases, ghost AI tightens)
- Score display using `std::format`
- SDL2 audio for pellet eating, ghost death, game over
- High score persistence

**C++20 focus:** `std::format` (opportunistic)

**Success criterion:** a person who has never seen the codebase can pick it up, play it, and have fun.

---

## Module graph

```
main.cpp
├── import engine.game
├── import engine.renderer
└── import engine.input

engine.game
├── import engine.renderer
└── import engine.input

engine.renderer
└── import <SDL.h>

engine.input
└── import <SDL.h>
```

`main.cpp` is a wiring file. It contains no logic — only three imports and a game loop. The game loop itself is three lines.

---

## Project structure

```
pacman/
├── PROJECT.md          ← you are here
├── CMakeLists.txt
└── src/
    ├── main.cpp
    └── engine/
        ├── game.ixx
        ├── renderer.ixx
        └── input.ixx
```

Module interface units use the `.ixx` extension (MSVC convention). Implementation units for the same module use `.cpp` with `module engine.x;` at the top (no `export`).

---

## Design principles

These apply to every line of code written in this project.

- **No forced features.** If a C++20 feature does not make the code cleaner or more correct in a given context, do not use it there. Every usage must be defensible.
- **No virtual dispatch in the entity system.** Concepts replace inheritance hierarchies. If you find yourself writing a base class, stop and ask whether a concept is the right tool.
- **`main` is a table of contents.** It should read like a high-level description of the program, not contain logic.
- **Module boundaries are architectural decisions.** Adding an import is not free — it expresses a dependency. Think before adding one.
- **The game must be fun.** Ghost AI personalities are not optional polish. They are what makes Pac-Man Pac-Man.

---

## Human-AI collaboration

This project is also a deliberate experiment in working with AI assistance. The goal is not to avoid AI, nor to over-rely on it — but to develop a principled, conscious relationship with it that is consistent with the project's core intent: reconnecting with the craft, proving sharpness, satisfying intellectual curiosity.

The framework used here is based on the concept of **delegation** — being explicit about what cognitive work is handed off to AI, and what is retained.

### The core principle

**Delegate the container. Own the content.**

The container is everything surrounding the interesting problem — build system wiring, SDL2 initialization, boilerplate, API surface. Delegating this costs nothing worth keeping.

The content is the reason this project exists — the coroutine scheduler, the Concepts design, the ghost AI, the architectural decisions. This stays with the author.

A practical test: **if the task requires thinking in C++20, own it. If it requires remembering an API, delegate it.**

### The four delegation tiers

**Tier 1 — Author drives, AI is silent.**
The author writes the code and makes all design decisions independently. AI is not consulted until a review is explicitly requested. This tier applies to every C++20 feature implementation, every architectural decision, the ghost AI logic, and the entity system design.

**Tier 2 — Author drives, AI navigates.**
The author writes the code, but may ask questions mid-implementation. "Does this coroutine structure make sense?" "Am I misusing Concepts here?" AI responds to what has been built — it does not build in place of the author. This tier applies to anything where a C++20 feature is being learned in real time.

**Tier 3 — AI drafts, author owns.**
AI produces a starting point — boilerplate, CMake configuration, SDL2 wiring, class skeletons. The author reads every line, understands it, modifies what does not feel right, and commits it only when it genuinely belongs to them. This tier applies to toolchain setup, rendering infrastructure, and anything that is ceremony rather than craft.

**Tier 4 — Full delegation.**
AI handles it, author moves on. This tier applies to debugging cryptic compiler errors with no learning value, documentation formatting, and repetitive patterns already understood from a prior instance.

### Accountability — commit tagging

Every commit is tagged with its delegation tier:

```
[T1] Implement coroutine task scheduler
[T2] Fix ghost scatter timing — consulted AI on co_await semantics
[T3] SDL2 renderer boilerplate
[T4] Fix MSVC module scanning error in CMakeLists
```

At the end of the project, the git log tells an honest story of where the author drove and where they delegated. It also enforces honesty in the moment — before asking AI to write something, the tier must be consciously chosen.

---



- [C++20 Coroutines — cppreference](https://en.cppreference.com/w/cpp/language/coroutines)
- [C++20 Modules — cppreference](https://en.cppreference.com/w/cpp/language/modules)
- [C++20 Concepts — cppreference](https://en.cppreference.com/w/cpp/language/constraints)
- [SDL2 documentation](https://wiki.libsdl.org/SDL2/FrontPage)
- [The Pac-Man ghost AI — The Pac-Man Dossier](https://www.gamedeveloper.com/design/the-pac-man-dossier)
- [CMake FILE_SET for modules](https://cmake.org/cmake/help/latest/command/target_sources.html)

---

*Started April 2026. Built for learning, written with care.*
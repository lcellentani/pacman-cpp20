# Ghost House Release — Design Plan

## Context

Right now every non-Blinky ghost (`Ghost::behavior`, `ghost.cpp:195-237`) walks straight out
of the house at game start with no gate at all — `path_for_ghost()` runs immediately. There is
no mechanism for "stay in the house until X," which blocks two things the project needs:

1. Arcade-correct release order at game start (Pinky immediately, Inky after 30 dots, Clyde
   after 60 dots, with a force-release fallback if Pac-Man stalls).
2. A way for a ghost that's been eaten to return to the house and leave again later — deferred
   here, but the release mechanism should be shaped so that future work can call it a second
   time instead of duplicating it.

`docs/ghost-ai-spec.md:198-216` already specifies the arcade rule and explicitly flags it as
simplifiable for Phase 3. We're choosing **not** to simplify the release rule itself (real dot
counter, not a flat timer-per-ghost) but **are** simplifying the personal-counter/death-reset
half of the arcade rule, since Pac-Man death doesn't exist yet (Phase 4).

**Per `docs/WORKING_AGREEMENT.md`, ghost AI logic is T1/T2 by default** — this document is a
design spec for you to implement, not an instruction list for me to execute. Use it as the
reference while writing the code; ask if any part of the shape below turns out to be wrong once
you're in the coroutine code.

## Decisions made together

| Question | Decision |
|---|---|
| Release rule | Real dot counter (arcade-accurate), not flat per-ghost delays |
| Personal counters / death-reset | Deferred — global counter only, until Phase 4 adds lives/death |
| Dead-state return-to-house trigger | Deferred — only shape the release mechanism to be reusable |
| New `GhostState::House` | Yes, add it now |
| Bounce animation while waiting | Deferred — no visual movement, just hold position |
| Force-release arbitration | Centrally arbitrated — exactly one ghost released per timer expiry, in fixed order (Pinky → Inky → Clyde), not independent per-ghost checks |

## Architecture

### `game.types` (`src/game/types.ixx`)

- `GhostState`: add `House`. Becomes `{ House, Scatter, Chase, Frightened, Dead }`.
- `GameState` (currently `pacman_tile`, `pacman_dir`, `blinky_tile`): add
  - `int dots_eaten = 0;` — global counter, incremented once per pellet/super pellet cleared.
  - `float dot_timer = 0.0f;` — seconds since the last dot was eaten; drives force-release.
  - `std::optional<GhostId> next_force_release = GhostId::Pinky;` — who gets force-released
    next if the timer expires; advances Pinky → Inky → Clyde → `nullopt` once Clyde is out.
    (Pinky's own threshold is 0, so in practice it clears almost instantly and this field
    settles on Inky almost immediately — that's fine, it's still correct.)

### `game.config` (`src/game/config.ixx`/`.cpp`)

Add tunable thresholds, matching the existing `pacman_speed`-style runtime-tunable fields and
the spec's note that thresholds should be exposed via `GameConfig`:
- `int ghost_house_dots_inky = 30;`
- `int ghost_house_dots_clyde = 60;`
- `float ghost_house_force_release_seconds = 5.0f;`

### `game.ghost` (`src/game/ghost.ixx`/`.cpp`)

**New awaitable**, alongside the existing `wait_for` / `move_to` / `walk_path` (`ghost.cpp:19-115`):

```
struct wait_for_release {
    Ghost& ghost_;
    Scheduler& scheduler_;
    GameState& game_state_;
    int dot_threshold_;
    float force_release_seconds_;
    // same await_ready/await_suspend/await_resume boilerplate as the other three

    void update(float dt) {
        if (game_state_.dots_eaten >= dot_threshold_) { resume; return; }
        if (game_state_.next_force_release == ghost_.id()
            && game_state_.dot_timer >= force_release_seconds_) {
            advance game_state_.next_force_release to the next ghost in order (or nullopt);
            game_state_.dot_timer = 0.f;
            resume;
        }
    }
};
```

No per-frame visual callback parameter for now (bounce deferred) — but keep this awaitable
single-purpose like its siblings rather than overloading `wait_for`'s fixed-delay semantics, so
adding a callback slot later is additive, not a rewrite.

**Factor the wait+exit sequence into its own coroutine** so it can be called again later from a
future Dead-state branch without duplicating logic:

```
Task Ghost::release_from_house(Scheduler& scheduler, int dot_threshold) {
    state_ = GhostState::House;
    co_await wait_for_release(*this, scheduler, *game_state_, dot_threshold, force_release_seconds_);
    co_await walk_path(*this, scheduler, path_for_ghost(id_));
}
```

`behavior()` (`ghost.cpp:195-237`) changes from the unconditional `walk_path` call to:

```
if (id_ != GhostId::Blinky) {
    co_await release_from_house(scheduler, dot_threshold_for(id_));
}
while (true) { /* existing scatter/chase loop, unchanged */ }
```

`dot_threshold_for(GhostId)` is a small static helper next to `path_for_ghost`/
`scatter_target_for_ghost` (`ghost.cpp:287-304`): 0 for Pinky, `config.ghost_house_dots_inky` for
Inky, `config.ghost_house_dots_clyde` for Clyde. Ghost will need access to `GameConfig` for this
and for `force_release_seconds_` — thread it the same way `Stage` threads `config_.pacman_speed`
into `Pacman` today (a value passed in, not a stored reference), most likely widening
`Ghost::reset(...)`'s parameter list.

**Reusability for the deferred Dead path**: when that work happens, the Dead branch (wherever it
breaks out of the scatter/chase loop) reaches the house entrance and calls
`co_await release_from_house(scheduler, /*dot_threshold=*/0)` again — immediate release, no
second dot-gating, matching the spec ("re-joins play... in the previously active mode"). Nothing
in this plan commits to *how* that branch is structured; that's explicitly deferred.

**Debug visibility**: `Ghost::debug_state()` currently hardcodes `state` to `Chase`
(per `CLAUDE.md`'s open-items note). Worth fixing as part of this work — otherwise `House` will
exist but never show up in the debug overlay, making this hard to verify visually.

### `game.stage` (`src/game/stage.ixx`/`.cpp`)

- `Stage::update`: accumulate `game_state_.dot_timer += dt;` every frame.
- Pellet-clear site (`stage.cpp`, ~lines 53-58, where `map_.clear_tile()` runs on
  `Pellet`/`SuperPellet`): increment `game_state_.dots_eaten++;` and reset
  `game_state_.dot_timer = 0.f;`.
- `Stage::reset()`: reset the new `GameState` fields (`dots_eaten = 0`, `dot_timer = 0.f`,
  `next_force_release = GhostId::Pinky`) alongside the existing per-entity resets.

## Verification

1. `/build` (clang-ninja-debug preset) — confirms the module changes compile cleanly.
2. Fix/confirm `debug_state()` surfaces real `state_`, then run the game with the debug overlay
   (`D`) open:
   - Pinky should leave the house almost immediately.
   - Inky should stay in `House` state until roughly 30 pellets are eaten.
   - Clyde should stay until roughly 60.
3. Force-release check: park Pac-Man without eating a pellet for >5s right after start (or right
   after Pinky/Inky clears) and confirm the next queued ghost leaves even though its dot
   threshold hasn't been reached, and that only one ghost leaves per expiry.
4. Optional: a `game.console` dev command to print `dots_eaten` / `dot_timer` /
   `next_force_release` would make step 3 much easier to verify without counting pellets by eye —
   your call whether that's worth adding now.

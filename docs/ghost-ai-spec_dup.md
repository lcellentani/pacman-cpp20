# Ghost AI Specification

Source: The Pac-Man Dossier (Jamey Pittman) — the authoritative reverse-engineered
reference for the original Namco arcade game. This document distils the rules that apply
to this project's 28×31 tile map (`MAP_COLS=28`, `MAP_ROWS=31`, `TILE_SIZE=24`).

---

## State machine

Every ghost runs the same state machine. The active state determines which target tile the
ghost pursues.

```
  ┌─────────────┐                                   ┌─────────────┐
  │             │──── timer (7 s / 5 s) ───────────►│             │
  │   Scatter   │                                   │    Chase    │
  │             │◄─── timer (20 s / ∞)  ────────────│             │
  └──────┬──────┘                                   └──────┬──────┘
         │  power pellet                                   │  power pellet
         └─────────────────────┬───────────────────────────┘
                               │
                               ▼
                       ┌───────────────┐
                       │               │──── timer expires ────► resume mode
                       │  Frightened   │
                       │               │
                       └───────┬───────┘
                               │ Pac-Man eats ghost
                               ▼
                       ┌───────────────┐
                       │     Dead      │──── reached home ─────► resume mode
                       │    (Eyes)     │
                       └───────────────┘
```

"Resume mode" means the ghost returns to whichever of Scatter or Chase was active when
the interruption occurred. The mode timer continues from where it was paused.

On every **mode change** (Scatter↔Chase, and entering Frightened) all ghosts immediately
reverse direction. This is the only moment reversal is allowed.

---

## Movement rules

### At every tile centre
1. Evaluate all neighbours that are not walls.
2. Remove the reverse direction (the tile the ghost just came from).
3. Pick the neighbour whose tile centre is closest (Euclidean distance) to the target tile.
4. Ties broken by priority: **Up > Left > Down > Right**.

### No-up zones
Two T-intersections inside the main corridors forbid the Up direction, preventing ghosts
from taking shortcuts through the ghost house area. In the 28×31 map these are located at
approximately:

| Zone | Col | Row |
|------|-----|-----|
| Left  | 12 | 11 |
| Right | 15 | 11 |

> Verify exact tiles against the hardcoded map layout in `game.map` before implementation.
> The defining visual cue: the two T-junctions directly below the ghost house entrance.

### Tunnel
The horizontal wrap-around corridor (row 14) slows ghosts to **40 %** of their normal
speed while they are inside it.

### Ghost house
Ghosts inside the house bounce vertically. They do not use the standard tile-decision
logic until released. On release they move to the house exit tile and join normal play
heading left.

---

## Scatter corners

In Scatter state each ghost ignores Pac-Man and homes in on a fixed corner of the maze,
causing them to loop endlessly. The corners are outside (or at the edge of) the playfield.

| Ghost | Target col | Target row | Corner |
|-------|-----------|-----------|--------|
| Blinky (red)   | 25 |  0 | Top-right    |
| Pinky (pink)   |  2 |  0 | Top-left     |
| Inky (cyan)    | 27 | 30 | Bottom-right |
| Clyde (orange) |  0 | 30 | Bottom-left  |

---

## Chase target tiles

### Blinky — direct pursuit
Target = Pac-Man's current tile.

The simplest and most aggressive personality. Blinky trails Pac-Man directly, which at
high speed creates a relentless pursuer.

### Pinky — ambush
Target = 4 tiles ahead of Pac-Man's current facing direction.

```
Pac-Man facing Right at (col, row) → target = (col+4, row)
Pac-Man facing Down  at (col, row) → target = (col,   row+4)
Pac-Man facing Left  at (col, row) → target = (col-4, row)
Pac-Man facing Up    at (col, row) → target = (col-4, row-4)  ← overflow bug (see note)
```

**Overflow bug note:** in the original arcade, the "4 tiles ahead" calculation used
unsigned arithmetic. When Pac-Man faces Up the result overflows, producing a target
4 tiles up *and* 4 tiles left. Replicating this bug is optional but preserves the
authentic feel — Pinky's ambush from above is less effective in the original partly
because of it. Recommended: **replicate the bug** for authenticity.

### Inky — flanking / unpredictable
Target derived from both Pac-Man's position and Blinky's position:

1. Compute the **pivot** tile: 2 tiles ahead of Pac-Man's facing direction
   (same overflow behaviour as Pinky, but 2 tiles instead of 4).
2. Draw a vector from Blinky's tile to the pivot.
3. Double that vector. The tip is Inky's target.

```
target = pivot + (pivot - blinky_tile)
       = 2 * pivot - blinky_tile
```

When Blinky is far away, Inky targets a distant point; when Blinky is close behind
Pac-Man, Inky converges on Pac-Man too. This creates a pincer effect.

### Clyde — bashful / proximity-aware
- If distance from Clyde to Pac-Man **> 8 tiles**: target = Pac-Man's tile (same as Blinky).
- If distance from Clyde to Pac-Man **≤ 8 tiles**: target = Clyde's **scatter corner** (bottom-left).

Distance is tile-to-tile Euclidean (not Manhattan). The 8-tile threshold means Clyde
retreats whenever he gets too close, making him appear timid.

---

## Scatter / Chase timing (Level 1)

The global mode timer cycles through fixed durations. All four ghosts switch
simultaneously (each reverses direction on the switch).

| Phase | Mode    |  Duration  |
|-------|---------|------------|
| 1     | Scatter | 7 s        |
| 2     | Chase   | 20 s       |
| 3     | Scatter | 7 s        |
| 4     | Chase   | 20 s       |
| 5     | Scatter | 5 s        |
| 6     | Chase   | 20 s       |
| 7     | Scatter | 5 s        |
| 8     | Chase   | indefinite |

A Frightened interruption pauses the mode timer. When Frightened ends, the timer
resumes from where it left off (it does not reset).

Higher levels shorten or eliminate the later Scatter phases. For Phase 3 scope: implement
Level 1 timing only and surface the durations via `GameConfig` for later tuning.

---

## Frightened mode

Triggered when Pac-Man eats a power pellet (super pellet in this codebase).

- All ghosts enter Frightened simultaneously.
- Ghosts reverse direction immediately on entry.
- Movement: **random** — at each tile centre, pick any non-reversing passable neighbour
  at random (no target tile). No-up zones still apply.
- Speed: 50 % of normal.
- Duration (Level 1): **6 seconds** of full frightened, then **5 flashes** (visual warning),
  then mode expires and ghosts resume Scatter or Chase from wherever the timer was.
- If Pac-Man eats another power pellet while already Frightened, the timer resets.

### Eating a ghost
When Pac-Man overlaps a Frightened ghost:
- Ghost enters Dead (Eyes) state immediately.
- Ghost is no longer a threat.
- Pac-Man freezes briefly (the score popup moment — implement in Phase 4).

---

## Dead (Eyes) state

Target = the ghost house entrance tile (approximately col 13, row 11 in the 28×31 map —
verify against the map layout).

- Ghost moves at **full speed** (or faster — original uses ~1.5× normal).
- Ghost ignores Pac-Man; it cannot eat nor be eaten.
- No-up zones still apply.
- On reaching the entrance tile the ghost re-enters the house, briefly bounces, then
  re-joins play in the previously active global mode (Scatter or Chase).

---

## Ghost release order

Ghosts start inside the house and are released based on a **dot counter** shared across
all four ghosts. Release order and dot thresholds for Level 1:

| Ghost | Condition |
|-------|-----------|
| Blinky | Starts outside the house, released immediately |
| Pinky  | Released immediately (exits the house at game start) |
| Inky   | Released after 30 dots eaten |
| Clyde  | Released after 60 dots eaten (roughly 1/3 of total pellets) |

A secondary **personal dot counter** per ghost resets if Pac-Man dies; the global counter
does not (Level 1 behaviour). A **force-release timer** (~5 s of Pac-Man inactivity) ejects
the next waiting ghost regardless of dot count.

> For Phase 3 scope, the dot counters and force-release timer can be simplified or deferred
> — the important thing is that ghosts exit in the correct order and the house entrance
> logic works. Exact thresholds can be tuned later via `GameConfig`.

---

## Speed reference (Level 1, relative to a fixed pixel/second base)

| Entity / situation   | Speed (% of base) |
|----------------------|-------------------|
| Pac-Man (normal)     | 80 %              |
| Pac-Man (eating dot) | 71 %              |
| Ghost (normal)       | 75 %              |
| Ghost (tunnel)       | 40 %              |
| Ghost (frightened)   | 50 %              |
| Ghost (dead / eyes)  | 150 %             |

The "base" speed in the original is 75.75 pixels/second at 60 Hz. For this project,
expose the ghost normal speed via `GameConfig` (already has `pacman_speed`) and derive
the other speeds as fixed ratios at runtime.

---

## Scope notes for Phase 3

| Rule | Include | Notes |
|------|---------|-------|
| Scatter / Chase / Frightened / Dead states | Yes | Core loop |
| Per-personality chase targets | Yes | Blinky, Pinky, Inky, Clyde |
| Pinky overflow bug | Yes (recommended) | Authenticity |
| No-up zones | Yes | Needed for correct navigation |
| Scatter/Chase timer (Level 1) | Yes | Expose via `GameConfig` |
| Ghost release dot counters | Simplified | Exact thresholds deferred |
| Force-release timer | Simplified | Can be a fixed delay for now |
| Tunnel speed reduction | Yes | Map query needed |
| Multi-level timing variation | No | Phase 4 scope |
| Pac-Man freeze on ghost eat | No | Phase 4 (score popup) |
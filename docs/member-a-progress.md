# Member A progress - Milestone 2 gameplay increment

Updated: 2026-11-05 (Australia/Sydney).

## Scope and milestone boundary

ProjectPlan.pdf assigns A gameplay, progression and game state. This M2 increment
fills A's missing M1 combat prerequisites and implements EXP/weapon selection on
B's current deterministic M1 map. It does not claim the team's whole M2 is done.

| Milestone | A deliverable | Current status |
| --- | --- | --- |
| M1 / Week 7 | Movement, one enemy type, shooting and collision | Implemented; supplied player movement retained and tested |
| M2 / Week 8 | EXP attraction, level-up/weapon selection, gameplay state | Implemented with temporary English HUD and modal menus |
| M3 / Week 9 | Weapon-hit particles, final integration and report evidence | Pending; shot/hit/blast interfaces supplied |

B still owns Day/Night maps and lighting; C owns final models/animation, camera,
HUD/menu presentation, instancing and comparative benchmark evidence. Coloured
projectiles and the blast-radius ring are functional proxies, not M3 particles.

## Current gameplay

- Supplied camera-relative WASD supports eight directions, normalized diagonal
  speed, block collision and sliding. Scene integration also blocks entry into
  enemies. Player default speed remains 5 blocks/second.
- Spawn one enemy every two simulation seconds, cap 24, reuse B's entry metadata,
  require four blocks of player separation and validate live blocks/actor
  occupancy. One enemy type has **4 HP** and moves at **4.3 blocks/second**.
- Enemy navigation uses four-neighbour BFS with live actor occupancy, varied
  tie directions and persistent waypoints. Blocked routes are replanned; retaining
  the waypoint through a grid-boundary crossing fixes the previous centre/turn
  oscillation. Short movement substeps prevent stepping through walls/actors.
  Crowding still queues naturally in narrow lanes; no actor overlap is allowed.
- Left-button press edges shoot towards the cursor, independent of player
  facing. Logical window coordinates are unprojected through C's camera onto
  Y=1.32. ImGui capture suppresses gameplay input. Holding is not automatic fire.
- Swept projectile collision prevents tunnelling. Each projectile retains its
  launch-time weapon/speed/damage. Wall collisions win ties with enemy hits.

| Weapon | Damage | Minimum shot interval | Projectile speed | Behaviour |
| --- | --- | --- | --- | --- |
| Balance | 2 | 0.30 s | 24 blocks/s | Stops on first enemy/wall; two hits kill |
| Laser | 1 | 0.12 s | 40 blocks/s | Pierces enemies, reflects off one wall, stops on the next; four hits kill |
| Bomb | 4 | 1.20 s | 8 blocks/s | Explodes on first enemy/wall; full damage within 3.5 blocks; one blast kills |

A Laser projectile damages each enemy at most once for its whole lifetime,
including its reflected path. It can still damage other enemies after passing
through one. Bomb radius uses enemy-centre distance in X/Z, has no falloff and
also affects enemies behind cover inside the radius; own bombs do not damage the
player. A Bomb reaching its three-second lifetime also detonates. Temporary
rendering uses yellow bullets, cyan laser streaks, orange bombs and a brief
orange ring marking the full blast radius.

- Kills drop one green EXP proxy. Within four blocks it attracts at seven
  blocks/second, collects within 0.65 blocks and intentionally ignores cover.
- Threshold is `current level * 3` EXP. Level-up freezes movement, enemies,
  spawning and timer and opens a centred, dimmed-background English modal.
  **Two distinct weapons are randomly offered** from Balance/Laser/Bomb.
  Selection equips one, consumes EXP, increments level, restores all 5 HP and
  resumes (or offers
  another level if enough EXP remains). Damage no longer grows with level, so
  the requested 2/4/1 hit counts remain valid at every level.
- Player has 5 HP and one-second invulnerability after contact damage. Survive
  180 seconds to win; lose at zero HP. Loss wins a same-frame tie. Terminal
  states stop simulation and show a centred **VICTORY** or **DEFEAT** modal with
  kills, level, survival time and **Play again**. Restart resets player/gameplay
  together, preserving the current map. None of these menus depend on saved HUD
  sizes; choices cannot be dismissed into an unexplained paused state.

## Interfaces for teammates

### Member B: map and collision

`Gameplay::update(dt, playerPosition, world, optionalShotTarget)` reads the live
world without owning or editing it. `spawnEnemy(position, player, world)` checks
solid clearance, distance, enemy occupancy and cap. `enemyHalfExtents()` is
`(0.245, 0.308, 0.245)`; actor centres rest at Y=1.32. Live map changes are checked
while moving/replanning. Continue calling `ArenaScene::rebuildWorldGeometry()`
after block edits. Keep the player spawn clear for restart.

Day/Night selection remains disabled until distinct layouts are supplied.
Coordinate with C on replacing `ArenaWorld(layout)`, rebuilding the mesh and
resetting gameplay/player together. Do not retain old spawn metadata for a new
layout. Bomb blasts currently do not destroy map blocks.

### Member C: models, animation, HUD and system integration

- `ArenaScene::gameplay()` exposes simulation. `stats()` supplies phase, HP,
  timer, level, EXP, selected weapon and kills.
- Read-only `enemies()`, `bullets()` and `orbs()` snapshots provide positions.
  Enemy IDs remain stable until reset; navigation waypoints are gameplay-owned.
  Replace `ArenaRenderer::drawProxy` with final actors/instances as needed.
- `shots()` contains launch position/direction/weapon; `hits()` contains impact
  position/enemy ID/killed/weapon. Both clear on every simulation update; consume
  before the next update and never retain references across update/reset.
- `explosions()` contains active blast positions, radii and ages (0.4-second
  lifetime). `drawBlast` currently draws a functional circle. A's M3 particles
  can use the same hit/blast data without changing collision ownership.
- `weaponProfile` and `weaponDescription` share the simulation/UI contract.
  Current enum values are `Balance`, `Laser`, `Bomb`; `Bullet` also exposes
  reflection count and already-hit IDs for debugging.
- `upgradeChoices()`/`chooseUpgrade(index)` own random offers and the resume
  boundary. C can replace `GameplayUi` with the final menu while keeping it.
  `drawGameplayUi` now returns a restart request; the caller must invoke
  `ArenaScene::resetGameplay()` so both simulation and player reset together.
- `aimOnPlane` uses logical cursor/window dimensions and the actual framebuffer
  view-projection; preserve that camera/HiDPI convention for a future crosshair.
- Benchmark timing boundaries remain unchanged. Command-line benchmark starts
  before spawning and freezes the static scene under `milestone2-local-follow-arena`.
  In-game benchmark freezes the current actor snapshot under
  `milestone2-local-follow-gameplay-frozen`; draw-call counts include blast rings. This is
  smoke evidence, not baseline-versus-instancing performance evidence.

## Validation

Linux cloud instance: GCC 14.2, CMake 3.31, Mesa llvmpipe software rendering.

```bash
source /workspace/.setup-9415/env.sh  # cloud-local dependencies only
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
xvfb-run -a -s '-screen 0 1280x720x24' \
  env LIBGL_ALWAYS_SOFTWARE=1 CRAFT_BENCHMARK=1 ./build/Craft
```

- Full application/test build passed; **21/21 CTest cases passed**. Coverage
  includes B's map tests, movement/aim, spawning, crowd rerouting through cover,
  enemy speed, EXP/selection/reset, terminal states, weapon speeds/cooldowns,
  exact 2/4/1 hit counts after selection, laser penetration/one reflection and
  bomb impact/AOE with full damage and out-of-range survival.
- Separate ImGui interaction harness exercised actual upgrade and victory/defeat
  modals at 640x480, 1280x720 and 1920x1080, even with Arena status collapsed:
  menus fit without scrolling, outside clicks do not dismiss the upgrade,
  choosing resumes, and Play again requests a reset and closes the result menu.
- Xvfb/XTest input/render smoke injected Enter, WASD, mouse clicks and Escape;
  screenshots inspected and executable exited 0. The actual defeat dialog and
  Play again were exercised: HP reset to 5, time to roughly 180, kills/EXP to
  zero and player to the central spawn. The command-line benchmark
  completed 60 warm-up + 240 measured frames and exited 0.

For a workstation: configure `cmake -S . -B build -DBUILD_TESTING=ON`, build/test,
then run `./build/Craft` (Windows executable path depends on the generator).
On Windows, the earlier C++20 designated initializer in `SceneBenchmark` was
replaced with C++17-compatible assignments; no standard upgrade is required.

Manual Windows playtesting/balance, CSE/VLAB and final visual acceptance remain
pending. Day/Night maps, final models/HUD, crosshair presentation and M3 particle,
normal-mapping and BRDF challenges are not claimed here.

## Team handoff

> Member A / M2: combat and EXP/selection run on B's arena. Added explicit
> upgrade/result modals and full-scene restart, faster occupancy-aware enemy
> navigation and Balance/Laser/Bomb behaviours. Level-up offers two random
> distinct weapons. B's collision/spawn interfaces are preserved; C can consume
> actor/shot/hit/blast snapshots for final presentation and instancing. Build,
> 21 tests and UI/render checks passed; Windows balance and CSE acceptance remain
> follow-up work. M3 particles are still deferred.


## Local follow camera and smaller grounded actors

The latest requested view shows the player's neighbourhood, with a fixed
orthographic half-height of 8 blocks. Camera offset is 7 blocks horizontally and
26 vertically (about 15 degrees from vertical). Player position is always the
view target; zoom does not change near map edges or adapt to fit the arena.
Distant scene areas fall outside the normal view clipping volume. No Day/Night
lighting, fog or exploration system is implied by this camera change.

Player and enemy models use 70% of the old world scale. Shared `ActorGeometry.h`
provides the render scale, grounded actor-centre height (1.32), player extents
(0.294, 0.308, 0.294), enemy extents (0.245, 0.308, 0.245), and separation values.
Player feet and enemy base remain above the floor at Y=1. Actor/world collision,
actor separation, contact reach, bullet hits and the aim plane use the new
proportions. B's map/metadata is unchanged: player and enemy spawn consumers take
its X/Z and ground the smaller body at the shared height. C should use these
shared values when replacing models; do not hardcode the old 1.45 height.

A valid upgrade selection fully heals to 5 HP, including successive level-ups;
an invalid choice does not heal or resume. The English modal explains the heal.
New tests cover wounded-player upgrade healing, actor scaling/grounding and
camera follow/scale/clipping/aim across landscape, portrait and ultrawide aspects.
All 21 tests passed; runtime screenshot confirms local cropped surroundings and
near-overhead rendering. The 240-frame render smoke passed with the new camera
workload label; timings cannot be compared directly with the older full-map view.
Windows visual comfort/zoom acceptance remains local playtesting work.

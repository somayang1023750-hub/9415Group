# Member A progress - Milestone 2 gameplay increment

Updated: 2026-11-12 (Australia/Sydney).

Status (2026-11-12): three typed monsters and Day/Night encounter ramps are
implemented on team baseline `8c1a587`. See [mixed enemy encounters](member-a-monsters.md)
for exact profiles, generation curves, rendering/navigation interfaces and
current validation. This latest monster increment does not complete weapon hit
particles or refresh the team's six pre-existing failing assertions.

The older sections below describe the baseline weapon/gameplay integration;
where they describe only one enemy kind, the linked monster specification is
the current source of truth.

## Scope and milestone boundary

ProjectPlan.pdf assigns A gameplay, progression and game state. This M2 increment
fills A's missing M1 combat prerequisites and implements EXP/weapon selection on
B's current deterministic M1 map. It does not claim the team's whole M2 is done.

| Milestone | A deliverable | Current status |
| --- | --- | --- |
| M1 / Week 7 | Movement, one enemy type, shooting and collision | Implemented; supplied player movement retained and tested |
| M2 / Week 8 | EXP attraction, level-up/weapon selection, gameplay state | Implemented with temporary English HUD and modal menus |
| M3 / Week 9 | Weapon-hit particles, final integration and report evidence | Pending; shot/hit/blast interfaces supplied |

B still owns Day/Night maps and lighting; C owns final models/animation,
HUD/menu presentation, instancing and comparative benchmark evidence. Camera and
actor proportions were adjusted in this A integration at Member A's request;
C retains the camera/presentation handoff for later integration. Coloured
projectiles and the blast-radius ring are functional proxies, not M3 particles.

## Current gameplay

- Supplied camera-relative WASD supports eight directions, normalized diagonal
  speed, block collision and sliding. Scene integration also blocks entry into
  enemies. Player default speed remains 5 blocks/second.
- Every two simulation seconds, use B's spawn candidates to attempt a batch,
  capped by 24 live enemies. Add one enemy to each batch every 30 elapsed
  gameplay seconds. Require four blocks of player separation and validate live
  block and actor occupancy. One enemy type has **4 HP** and moves at **3.3
  blocks/second**.
- Enemy navigation uses four-neighbour BFS with live actor occupancy, varied
  tie directions and persistent waypoints. Blocked routes are replanned; retaining
  the waypoint through a grid-boundary crossing fixes the previous centre/turn
  oscillation. Short movement substeps prevent stepping through walls/actors.
  Crowding still queues naturally in narrow lanes; no actor overlap is allowed.
- Holding the left mouse button fires every owned weapon independently at its
  own cooldown. Shots follow the mouse-projected world point and do not turn the
  player model. Logical window coordinates use C's camera and framebuffer size
  to project onto Y=1.32. ImGui capture suppresses gameplay input.
- Swept projectile collision prevents tunnelling. Each projectile retains its
  launch-time weapon/speed/damage. Wall collisions win ties with enemy hits.

| Weapon | Damage | Minimum shot interval | Projectile speed | Behaviour |
| --- | --- | --- | --- | --- |
| Bullet | 2 | 0.30 s | 5 blocks/s | Stops on first enemy/wall; two hits kill |
| Laser | 1 | 0.48 s | 40 blocks/s | Stops on first enemy; reflects off one wall |
| Bomb | 2 | 1.20 s | 8 blocks/s | Explodes on first enemy/wall; full damage within 1.75 blocks |

Bullet upgrades are speed ×2, damage ×2, speed ×2, then fire rate ×2. Its fifth
upgrade evolves it into a three-way shot with two smaller base-damage pellets
at ±30 degrees. Laser upgrades are damage ×2, interval ×0.5, ricochets +1, then
interval ×0.5. Its fifth upgrade evolves it into a red beam that pierces enemies
until its first obstacle bounce; after bouncing, it damages and stops at the
next enemy, even if that enemy was hit before the bounce. Bomb upgrades double
blast radius, add 5 blocks/second of flight speed, double damage and increase
fire rate by 50%. Its fifth upgrade evolves it into a firebomb with an orange-red
flight trail and a three-second burning area dealing 2 damage per second.

Bomb radius uses enemy-centre distance in X/Z, has no falloff and also affects
enemies behind cover inside the radius; own bombs do not damage the player. A
Bomb reaching its three-second lifetime also detonates. Firebomb burning areas
use the upgraded bomb radius.

- Kills drop one green EXP proxy. Within four blocks it attracts at seven
  blocks/second, collects within 0.65 blocks and intentionally ignores cover.
- Threshold is `current level * 3` EXP. Level-up freezes movement, enemies,
  spawning and timer and opens a centred, dimmed-background English modal.
  Up to two random choices are offered: an upgrade/evolution for an owned weapon
  or an unlock for Laser/Bomb. Unlocking adds a weapon to the arsenal. Selection
  consumes EXP, increments level, restores all 5 HP and resumes (or offers
  another level if enough EXP remains). All owned weapons fire together while
  the left mouse button is held.
- Player has 5 HP and one-second invulnerability after contact damage. Survive
  180 seconds to win; lose at zero HP. Loss wins a same-frame tie. Terminal
  states stop simulation and show a centred **VICTORY** or **DEFEAT** modal with
  kills, level, survival time and **Play again**. Restart resets player/gameplay
  together, preserving the current map. None of these menus depend on saved HUD
  sizes; choices cannot be dismissed into an unexplained paused state.

## Local follow camera and smaller grounded actors

The latest requested view shows the player's neighbourhood, with a fixed
orthographic half-height of 8 blocks. Camera offset is 7 blocks horizontally and
12 vertically (about 60 degrees above the arena plane). Player position is always the
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
Those test and render results were recorded before the latest weapon redesign;
they do not validate the current progression, progressive spawns or firebomb.
The 240-frame render smoke passed with the prior camera workload label; timings
cannot be compared directly with the older full-map view.
Member A has accepted the current gameplay in conversation; broader device and
resolution acceptance remains future integration work.

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
  `ArenaScene` now tracks enemy yaw and walk phase by ID, and
  `ArenaRenderer::drawZombie` draws a hierarchical six-part zombie with fixed
  forward arms and swinging legs. `Gameplay::attacks()` emits frame-local events
  when proximity damage is applied; the scene uses these to raise the attacking
  zombie's arms. `assets/textures/zombie.png` is the 64 × 64 skin atlas.
- `shots()` contains launch position/direction/weapon; `hits()` contains impact
  position/enemy ID/killed/weapon. Both clear on every simulation update; consume
  before the next update and never retain references across update/reset.
- `attacks()` contains the enemy ID, position and direction whenever proximity
  damage is actually applied. It is frame-local and drives the zombie arm-raise
  animation; the current one-second damage cooldown is shared globally.
- `explosions()` contains active blast positions, radii and ages (0.4-second
  lifetime); `burningAreas()` exposes firebomb position, radius and age for the
  scene's ground ring and rising particles.
- `weaponProfile` and `weaponDescription` share the simulation/UI contract.
  The enum is `Bullet` (`Balance` remains an alias), `Laser` and `Bomb`.
  `weapons()` reports owned-weapon upgrade/evolution state, while
  `upgradeOptions()` and `chooseUpgrade(index)` share the random card contract.
  `upgradeChoices()` remains a weapon-only compatibility view.
- `aimOnPlane` projects logical cursor/window dimensions with the actual
  framebuffer view-projection onto the actor-height plane. Shooting does not
  change the player model's facing; it uses the projected point directly.
- Benchmark timing boundaries remain unchanged. Command-line benchmark starts
  before spawning and freezes the static scene under `milestone2-local-follow-arena`.
  In-game benchmark freezes the current actor snapshot under
  `milestone2-local-follow-gameplay-frozen`; draw-call counts include blast rings. This is
  smoke evidence, not baseline-versus-instancing performance evidence.

## Validation

Linux cloud instance: GCC 14.2, CMake 3.31, Mesa llvmpipe software rendering.
The current reimplementation was compiled in the existing WSL build directory
with `cmake --build build --target Craft --clean-first --parallel 4`; all app
sources, including `FollowCamera.cpp`, were rebuilt successfully. CTest and
manual visual/gameplay acceptance were not run for this reimplementation.
The prior test and render results below were recorded before the latest weapon
redesign and do not validate its progression, progressive spawning or firebomb.

```bash
source /workspace/.setup-9415/env.sh  # cloud-local dependencies only
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
xvfb-run -a -s '-screen 0 1280x720x24' \
  env LIBGL_ALWAYS_SOFTWARE=1 CRAFT_BENCHMARK=1 ./build/Craft
```

- Full application/test build passed; **21/21 CTest cases passed**. Coverage
  comprises 16 A gameplay regression cases, four B world cases and the existing
  starter math case. Coverage includes movement/aim, spawning, crowd rerouting,
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

Member A ran the project on Windows/Visual Studio, reported the encountered
issues during iteration, and subsequently accepted the current gameplay in chat.
This user feedback is separate from the agent-run Linux checks above. A full
Windows automated test log, CSE/VLAB validation and final team visual acceptance
remain outstanding. Day/Night maps, final models/HUD, crosshair presentation and
M3 particle,
normal-mapping and BRDF challenges are not claimed here.

## Team handoff

> Member A / M2: combat and EXP/selection run on B's arena. Added explicit
> upgrade/result modals and full-scene restart, faster occupancy-aware enemy
> navigation and Balance/Laser/Bomb behaviours. Level-up offers two random
> distinct weapons. B's collision/spawn interfaces are preserved; C can consume
> actor/shot/hit/blast snapshots for final presentation and instancing. Build,
> 21 tests and UI/render checks passed; Member A accepted the gameplay after
> Windows iteration. Final team integration and CSE acceptance remain follow-up
> work. M3 particles are still deferred.

## Authorship and delivery record

The starter already supplied the start screen, six-part textured player and limb
swing, normalized WASD/block movement, camera scaffolding, ImGui lifecycle, mesh
helpers and benchmark harness. B supplied the configurable map and spawn
metadata. A reused these foundations; they are not claimed as entirely new A work.
A added gameplay systems, tests and integration, fixed the reported UX/build/AI
issues, and adjusted the camera/actor scale at the user's request.

| Commit | A delivery |
| --- | --- |
| `5cb86d4` | M1 combat prerequisites and M2 EXP/selection/state integration, tests and handoff |
| `fdd6266` | C++17-compatible benchmark snapshot initialization for the reported MSVC build failure |
| `ce52a0b` | Reliable centred upgrade selection and initial weapon differentiation |
| `aa53b10` | Balance/Laser/Bomb, live crowd routing, faster enemies, result/restart modals |
| `91e0e6f` | Upgrade full healing, smaller grounded actors and local overhead follow view |

Older Heavy/Rapid and level-based damage entries in CHANGELOG are historical;
the current pool/values are exclusively the Balance/Laser/Bomb table above.

| Code area | Ownership/interface supplied by this A work |
| --- | --- |
| `src/core/gameplay/Gameplay.*` | CPU simulation, weapons, enemies, progression, phases and events |
| `src/core/gameplay/ActorGeometry.h` | Shared render/collision/grounding proportions |
| `src/core/gameplay/Aim.h` | Camera-to-world aim conversion |
| `src/app/ArenaScene.*` | Player/world/simulation/render integration and complete restart |
| `src/app/GameplayUi.*` | Temporary English status, selection and outcome presentation |
| `src/app/FollowCamera.*` | Requested angled local follow view |
| `src/graphics/ArenaRenderer.*` | Textured hierarchical player/zombie, projectile/orb proxies and blast rendering |
| `tests/test_gameplay.cpp` | Gameplay, navigation, weapons, healing, geometry, movement and camera regression checks |

## Next work and acceptance boundary

| Owner | Remaining work |
| --- | --- |
| A / M3 | Weapon-hit particle bursts/styles using shot/hit/blast data; final integration and report evidence |
| B | Day/Night map configurations, planned lighting/material work and normal mapping |
| C | Polished HUD/menus and crosshair, instancing/BRDF and comparative performance evidence |
| Team | Wire level switching, integrate presentation, validate CSE/VLAB and target platforms, complete report/video |

Further weapon balance changes should preserve the currently agreed 2/4/1 hit
counts unless the team explicitly changes the design. Keep simulation pause,
heal-on-selection and full scene restart when replacing the temporary menus.
See [validation and replay guide](member-a-validation.md) for evidence boundaries,
Windows build instructions and the repeatable manual acceptance sequence.

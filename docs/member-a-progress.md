# Member A progress - Milestone 2 gameplay increment

Updated: 2026-11-05 (Australia/Sydney).

## Scope and milestone boundary

ProjectPlan.pdf assigns A core gameplay, progression and game state. The team
is currently at **Milestone 2**. The checked-in B increment still provides the
M1 deterministic map, and the starter already supplies camera-relative movement.
This change fills the missing A M1 combat prerequisites and implements A's M2
EXP/level-up loop on that map. It does not claim the team's whole M2 is complete.

| Milestone | A deliverable | Status in this increment |
| --- | --- | --- |
| M1 / Week 7 | Movement, one enemy type spawning/chasing, bullet collision | Existing movement retained and regression-tested; combat implemented |
| M2 / Week 8 | EXP attraction, level-up/random weapon choice, gameplay state | Implemented with a small functional integration panel |
| M3 / Week 9 | Weapon-hit particle effects and final integration/evidence | Pending; event interfaces supplied |

B's Day/Night maps and C's final character models/animations, HUD/menu styling,
instancing and report benchmark evidence remain their own integration work.

## Changes ready to share

- Added CPU-only `core/gameplay/Gameplay`, independent of GLFW, ImGui and OpenGL.
- Reuse B's live `enemySpawnCandidates()` and `canOccupy()`: spawn one enemy every
  two simulation seconds, cap at 24, require four blocks of player distance and
  reject solid blocks or overlapping enemies. Invalid/occupied candidates are
  skipped. Enemy centres and half-extents match the temporary render proxies.
- One enemy type has 2 HP and chases at 1.8 blocks/second. A four-neighbour BFS
  flow field navigates around cover and is rebuilt from live blocks; enemies
  cannot cut diagonal corners, enter solid blocks, or overlap live actors.
- Retain the supplied normalized eight-direction player movement and camera.
  Add player/enemy occupancy checking at the scene integration boundary.
- Logical window cursor coordinates are unprojected through C's existing camera
  onto Y=1.45. Left-button press edges fire a horizontal projectile; holding the
  button does not auto-fire. ImGui capture suppresses gameplay mouse/keyboard
  input. Shooting does not change the player's movement-facing direction.
- Swept segment/expanded-AABB collision selects the earliest enemy or wall hit;
  walls win ties. Bullets expire after two seconds, cannot pierce cover, and keep
  their launch-time damage/weapon even if a later upgrade changes the weapon.
- Kills drop one green EXP proxy. Within four blocks it attracts at seven
  blocks/second and collects within 0.65 blocks. Magnet attraction intentionally
  ignores cover; actors and bullets still respect cover. Formal glowing sphere
  visuals are left to C; these green boxes are integration proxies.
- Level threshold is `current level * 3` EXP. Crossing it pauses simulation,
  movement, spawning and countdown and offers two distinct shuffled choices from
  Balanced, Rapid and Heavy. Choosing equips a weapon, consumes the threshold,
  increments level, and handles any remaining EXP. Each level adds one damage.
  Base damage/cooldown: Balanced 1/0.25s, Rapid 1/0.12s, Heavy 2/0.5s. These are
  initial tuning values. No separate weapon models or M3 particle styles added.
- Added 5 player HP, one-second contact-damage invulnerability, a 180-second
  survival countdown, Won/Lost states, and restart of the current configuration.
  Loss takes precedence if health and countdown reach zero in the same update.
- Added `GameplayUi` as a replaceable functional panel for HP/time/kills/EXP,
  weapon offers and outcome text. Existing restart menu action now resets A's
  state and the player to B's spawn; it preserves the current map.

## Interfaces for teammates

### Member B: map/layout integration

`Gameplay::update(dt, playerPosition, world, optionalShotTarget)` uses the
current world without taking ownership of it. `spawnEnemy(position, player,
world)` validates solid clearance, distance and current enemy occupancy.
`enemyHalfExtents()` returns `(0.35, 0.44, 0.35)`; centres rest at Y=1.45.
Map edits are observed on the next simulation update. Continue calling
`ArenaScene::rebuildWorldGeometry()` after block edits; A never modifies blocks.

Day/Night selection remains disabled: only the M1 map is present. When B adds
layouts, agree with C on loading `ArenaWorld(layout)`, rebuilding the mesh and
resetting gameplay/player together. Do not reuse old spawn metadata for a new
layout. Restart expects the current map's player spawn to remain clear.

### Member C: rendering, animation, camera, HUD and integration

- `ArenaScene::gameplay()` exposes the simulation. `stats()` provides health,
  kills, EXP, level, selected weapon, remaining time and `Phase`.
- Read-only `enemies()`, `bullets()` and `orbs()` snapshots supply world positions;
  enemy IDs remain stable until reset. Replace `ArenaRenderer::drawProxy` with
  hierarchical actors/instanced draws without changing gameplay ownership.
- `shots()` supplies launch position/direction/weapon for recoil animation.
  `hits()` supplies position/enemy ID/killed/weapon for hit reactions and A's M3
  particle work. Both are frame-local, cleared on each `update`; consume before
  the next update and do not keep references across updates/reset.
- `upgradeChoices()` and `chooseUpgrade(index)` form the menu contract. Upgrade
  selection belongs in UI; simulation owns EXP, equipment and the paused phase.
  Replace `GameplayUi` with the planned HUD/menu while preserving that contract.
- `aimOnPlane` uses logical cursor/window dimensions and the actual framebuffer
  view-projection, including HiDPI scaling. Keep the same projection when
  changing C's camera. Movement-facing and mouse aim remain independent.
- Benchmark timing boundaries are unchanged. Simulation is frozen during a run.
  Command-line `CRAFT_BENCHMARK=1` starts with no actors and retains B's
  `milestone1-voxel-arena` label. An in-game run freezes the current gameplay
  snapshot and uses `milestone2-gameplay-frozen`; counts include proxy geometry.
  This is smoke validation, not baseline-versus-instancing performance evidence.

## Validation

On this Debian/Linux cloud instance, GCC 14.2, CMake 3.31, OpenGL via Mesa
llvmpipe (software rendering):

```bash
# Cloud instance local dependency activation; not needed on normal system installs
source /workspace/.setup-9415/env.sh
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
xvfb-run -a -s '-screen 0 1280x720x24' \
  env LIBGL_ALWAYS_SOFTWARE=1 CRAFT_BENCHMARK=1 ./build/Craft
```

- Full application/test build passed.
- CTest: 12/12 cases passed, including B's original world tests. New coverage:
  live spawn clearance/distance/occupancy/cap; chase around cover; swept bullet
  hits and cover blocking; fire cooldown/invalid aim; EXP attraction/collection;
  randomized distinct offers; pause/selection/reset; contact damage and win/loss;
  movement regression; camera-to-world aim round-trip with HiDPI dimensions.
- OpenGL command-line smoke: 60 warm-up + 240 measured frames, exit 0, static
  M1 map workload at 1920x1080. This does not validate GPU hardware performance.
- Xvfb/XTest interaction smoke: Enter transitioned into the arena, injected WASD
  and eight mouse clicks, rendered enemies/HP/countdown panel, then Escape exited
  0. Screenshot inspected. This is automated input/render validation, not a full
  manual playthrough or visual acceptance of the progression menu.

For a normal workstation: `cmake -S . -B build -DBUILD_TESTING=ON`,
`cmake --build build`, `ctest --test-dir build --output-on-failure`, then
`./build/Craft`. Enter starts; WASD moves; click towards red enemies, approach
EXP, choose a weapon on level-up, and use Restart current configuration.

Manual full-loop balance/usability checks, macOS/Windows portability and CSE/VLAB
validation are still pending. Multiple enemy models, polished EXP visuals,
Day/Night maps, final UI, and all three M3 challenge tasks are not claimed here.

## GitLab/team handoff

Suggested update:

> Member A / M2: added the missing M1 enemy/spawn/chase and click-to-shoot combat
> on B's arena, then EXP drops/magnet collection and paused randomized weapon
> selection. Added HP, survival outcome and restart, with temporary render/UI
> proxies. B's collision/spawn contracts are reused; C can consume snapshots and
> shot/hit events for models, HUD, animation and instancing. Particle effects are
> deferred to M3. Linux build, 12 CTest cases and OpenGL/input smoke checks passed;
> final visual acceptance and CSE verification remain pending.

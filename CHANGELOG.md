# Changelog

This file records the changes made to the starter code and explain why the issues happened and how we fixed them. Your own ideas and explanations should write in the report, or demonstrate in the short video.



## 2026-11-13 — Member A: revise early Night spawn cadence

- Night now attempts batches every 1.5 s during the first 30 active seconds.
  Over 30-60 s the interval rises linearly to 2 s, then remains at 2 s.
- Retained the Spider-first release sequence, 30:50:20 final generation weights,
  3-HP Spiders, normal batch-size growth and Day pacing.
- Updated real-cadence and monotonic transition checks; full build and 50 tests pass.

## 2026-11-13 — Member A: Spider health and Night encounter pacing

- Set Spider health to 3 HP (0.75x Zombie); retained damage, speed and EXP.
- Night starts Spider-only. Zombies ramp in over 30-90 s; Golems over 60-120 s.
  Final generation weights are Spider:Zombie:Golem = 30:50:20.
- Early Night attempts batches every 3 s before 30 active seconds, then every
  2 s. Day's curve/pacing, crowd routing, cap and six-block safety are retained.
- Full game/range build and 50 tests pass, including actual early spawn cadence,
  eligibility, pause/reset, weighted generation and the normal combat regressions.

## 2026-11-13 — Member A: unambiguous training-range launcher

- Renamed the executable output to MemberATrainingRange and added a v2 window/
  panel identity and compile timestamp. The CMake target remains compatible.
- Updated build_windows.cmd to build in build-training-range and automatically
  launch CMake's exact configuration-specific output, with source/commit/path
  diagnostics. Failed builds do not fall back to old executables.
- Updated capture instructions to remove ambiguous old Debug/root exe choices.

## 2026-11-12 — Member A: replace staged pages with an interactive range

- Replaced MemberAShowcase's replay/UI pages with a playable ArenaScene range.
  Five invulnerable, stationary models sit next to cover/outer walls; normal
  movement, aiming, actual projectile collisions and effects are retained.
- Added free per-weapon unlock/stage/evolution controls, SPACE time stop, N
  single stepping, F auto-stop on real impacts and PNG capture. Time stop skips
  all scene updates and retains existing collision/fire snapshots.
- Training-only rules are explicitly opt-in: no waves, contact attacks, target
  health loss or survival countdown. Default Craft instances keep normal rules.
- Normal game/range builds and 49 tests pass; live range smoke checks actual
  contacts, stationary health, frozen projectiles/fire, resume and GL rendering.

## 2026-11-12 — Member A: report capture workspace

- Added optional MemberAShowcase target and report/member-a with the revised Word
  report, screenshot instructions, and exports of real source for Figures A1–A5.
- Staged the production models side by side, real upgrade/victory/defeat UI,
  Bullet/Laser hits and the Bomb flight/blast/rotating fire sequence. Added PNG
  capture, pause and frame stepping; no substitute models or particle formulas.
- Extracted the original four-spark trail into a helper shared by ArenaScene and
  the showcase, preserving its formulas. Normal game and showcase builds, all
  48 tests and an OpenGL showcase smoke pass on Linux; Windows replay remains.

## 2026-11-12 — Member A: six-block spawn safety and stable crowd pursuit

- Reduced automatic spawn exclusion from eight to six blocks of body-edge
  clearance; all type/entry fallbacks still enforce the zone.
- Separated temporary actor congestion from static-corner recovery. Grid turns
  remain committed; blocked followers wait before a timed replan. Open-world
  crowd detours cannot begin by retreating, and only static corner disconnection
  permits cell-centre recovery. Direct chase still follows a moving player.
- Added side-by-side crowd and waiting/resuming queue regressions. Both fail
  against the previous navigation code and pass now; corner-recovery tests pass.
- Full application build and all 48 tests pass. Particle implementation remains
  the accepted `5af6183` version; progress and manual replay docs are updated.

## 2026-11-12 — Member A: restore the pre-edit firebomb appearance

- Restored the evolved Bomb flight-trail block verbatim from `e32b7cf`: original
  four lit sparks, sizes, colours, spacing and timing. Removed the later
  sixteen-spark emissive redesign.
- Retained the accepted outward explosion burst and restored the original ten
  rising ground-fire cubes/radius marker. Only their angular speed changes
  (0.8 to 3.2 rad/s) so the original particles themselves form the vortex.
- Kept three-second burning damage, Night fill, spawn safety and weapon rules.
  Updated draw/triangle counts for the restored lit rendering path.
- Full C++17 build and 46 tests pass; original-trail source comparison and
  Day/Night production-shader checks cover the corrected presentation.

## 2026-11-12 — Member A: safer encounters and rotating firebomb flames

- Added an eight-block player/enemy body-edge safety zone to all automatic spawn
  attempts, including type/entry fallbacks. Unsafe batches skip without reducing
  the distance. Existing explicit staging and map metadata contracts remain.
- Retained the accepted radial Bomb burst and restored the evolved Bomb's warm
  ground-fire appearance. The fire particles themselves orbit and rise into a
  vortex instead of using a flat red spiral overlay.
- Made the orange-red flight trail more visible with fading emissive particles
  in the shared instanced batch. Pauses/reset and weak local Night glow remain.
- Full application build, 46 tests and Day/Night production-shader render checks
  passed. Updated A's handoff and Windows replay checklist.

## 2026-11-12 — Member A: Windows effect-light build fix

- Included `<string>` explicitly in `ArenaRenderer.cpp` for `std::to_string`.
  The Night light uniform suffix previously relied on indirect standard-library
  includes, causing MSVC C2039/C3861 and cascading suffix-initialization errors.
  The fix retains C++17 and does not change gameplay or rendering behaviour.

## 2026-11-12 — Member A: corner recovery, impacts and Night effect lights

- Raised Golem health to 14 (3.5x Zombie); retained damage 2, EXP 2, speed and
  encounter curves. Enlarged its typed EXP drop from 0.14 to 0.25 half-extent.
- Reproduced rare corner grazing where sampled path clearance accepted a route
  movement rejected. Added continuous voxel sweeps, waypoint validation, safe
  current-cell connection and no-progress replanning without teleportation.
- Added Bullet debris, cyan/red Laser electrical contacts, radial Bomb bursts
  and evolved Bomb red ground vortices. Kept all team weapon/progression rules.
- Added weak, bounded Night impact/residue fill lights with distance/lifetime
  fade. Day lighting and distant surfaces stay unchanged.
- Preserved model instancing/normal-map attributes; particles use one dedicated
  instanced pass and independent RNG, with paused/reset lifecycles.
- Refreshed six obsolete baseline test cases and added actual unlock, separate
  cooldown, evolution, navigation and effect/light coverage. Full app build and
  all 43 tests pass; production-shader Day/Night GL/pixel checks pass.
- Updated [A progress](docs/member-a-progress.md), [effects handoff](docs/member-a-effects.md)
  and [validation](docs/member-a-validation.md). Windows/CSE replay remains open.

## 2026-11-12 — Member A: mixed Zombie/Golem/Spider encounters

- Added exact typed HP/damage/EXP, distinct body extents/speeds and original
  procedural Golem/Spider skins with biped/eight-leg hierarchical animation.
- Added smooth Day/Night generation weights and preserved the team batch/cap
  rules. Reset and level switching restart the selected map's encounter curve.
- Added body-aware swept collision and crowd-aware routing with persistent
  waypoints, front-first movement and wall-adjacent melee approach.
- Retained C's instancing by grouping models per type/part; updated diagnostics.
- Full app build and 12 new enemy tests passed. Full CPU suite retains the six
  known baseline failures (28/34 passed); see [enemy validation](docs/member-a-monsters.md).
- Weapon hit effects and the separate weapon/test redesign are unchanged.

## 2026-11-11 — Member B, M2 environments and M3 normal mapping

- Connected the seeded Day map and staggered Night barricades to the Game menu.
  Each layout supplies player and enemy spawns. Switching rebuilds the mesh
  and resets the run. Restart keeps the selected layout.
- Added a player-following point light with distance attenuation and cool ambient
  fill at night. Day retains the directional sunlight and Fresnel water shading.
- Added repeating face UVs and masonry textures on cover and walls, with
  distinct Phong highlights for sand, masonry, actors and metal projectiles.
  World batches use GPU back-face culling alongside exposed-face meshing.
- Added tangent-space normal maps for sand and masonry. The controls and
  benchmark output expose the normal-map setting for visual comparisons.
  See [Member B evidence](docs/member-b-progress.md).

## Current Member A status — 2026-11-05

Member A accepted the current gameplay after Windows/Visual Studio iteration.
Current code baseline is `91e0e6f`: Balance/Laser/Bomb, random two-choice level-up
with full healing, explicit result/restart menus, crowd rerouting, smaller
actors and local overhead following are documented in
[Member A progress](docs/member-a-progress.md). The current automated suite has
21 passing cases on Linux; CSE and full platform acceptance remain outstanding.

This documentation consolidation adds the
[validation and replay guide](docs/member-a-validation.md), aligns README with
current controls/values/Windows output paths, and records attribution and team
handoff. It changes no application code. Entries below retain implementation
history; older weapon pools, HP and workload labels are superseded by current
progress/README values.

## - 2026-10-05

Initial publish of the starter code for COMP3421/COMP9415 26T3 Computer Graphics group assignment to the staff repos from individual assignment repo.


## - 2026-11-03 — Member B: Milestone 1 map foundation

- Replaced the four starter pillars with eight cover groups, preserving a clear
  central spawn area and connected routes to the outer lane.
- Added CPU-only `ArenaLayout` data and an `ArenaWorld(layout)` constructor to
  separate map configuration from rendering and support later level layouts.
- Added the player spawn and eight enemy spawn candidates; `ArenaScene` now
  initializes the player from the map's spawn metadata. Enemy spawning logic
  remains Member A's responsibility.
- Added validation for obstacle bounds/heights and spawn clearance, plus tests
  for floor/boundary integrity, map connectivity, spawn reachability, collision
  compatibility, and invalid layouts.
- Changed the benchmark workload label to `milestone1-voxel-arena` to distinguish
  the new map from the starter; the benchmark measurement logic is unchanged.
- Updated the README and added [Member B progress and handoff](docs/member-b-progress.md)
  with integration interfaces, milestone boundaries, and verification details.

Validation: macOS build succeeded, all 5 CTest cases passed, and the OpenGL
application completed the 240-frame runtime check. Manual walking/visual checks
and CSE/VLAB verification remain pending.

Scope: this increment covers B's M1 map foundation only. Day/Night layouts,
UVs, materials, and lighting remain M2 work; normal mapping remains M3 work.

## - 2026-11-05 — Member A: Milestone 2 gameplay increment

- Reused existing eight-direction movement; added enemy spawning/chasing and
  swept click-to-shoot bullet collision on B's live arena map.
- Added EXP drops/magnetic collection, paused level-up with randomized weapon
  offers, equipment progression, HP/contact damage, survival outcomes and restart.
- Added CPU-only gameplay snapshots and shot/hit events for B/C integration;
  temporary cube actors and a functional panel leave final visuals/HUD to C.
- Preserved the static command-line benchmark; in-game frozen gameplay uses a
  separate workload label. Timing boundaries remain unchanged.
- Added seven gameplay/movement/aim test cases; all 12 CTest cases passed.
  Linux build, 240-frame OpenGL smoke and automated input/render smoke passed.
- Added [Member A progress and handoff](docs/member-a-progress.md). Day/Night map
  integration, final models/UI, CSE validation and M3 particles remain pending.


## - 2026-11-05 — Upgrade selection usability and weapon tuning

- Moved upgrade choices into a centred English modal with an explicit paused
  message, full-width buttons and post-upgrade weapon stats. Selection resumes
  gameplay; old/collapsed HUD sizes cannot hide the choices.
- Centralized weapon profiles: Heavy has slower shots/projectiles and higher
  damage; Rapid has faster shots/projectiles and lower damage; Balanced sits
  between them. Set enemy HP to 3 to distinguish required hits.
- Build and all 13 CTest cases passed; separate ImGui interaction checks verified
  the modal and resume flow at 640x480, 1280x720 and 1920x1080.


## - 2026-11-05 — Outcome menus, navigation and Balance/Laser/Bomb

- Added centred English victory/defeat modals with run statistics and a Play
  again request, routed through ArenaScene for full player/gameplay reset.
- Replaced turn-centre oscillation with persistent waypoints and live-occupancy
  rerouting. Enemy speed is now 4.3 blocks/s, just below the player's default 5.
- Replaced the old weapon pool with Balance/Laser/Bomb. Laser pierces enemies
  and reflects once; Bomb detonates on enemies/cover with full 3.5-block radial
  damage. Added distinct projectile proxies and a short-lived blast-radius ring.
- Enemy HP is 4 and damage is fixed at 2/1/4, preserving two/four/one hit kills
  after leveling. Every selection page offers two random distinct weapons.
- Build and 18 CTest cases passed, including navigation, exact hit counts,
  penetration, reflection, impact explosions and radial damage. UI checks cover
  upgrade/results and restart at three window sizes. Updated A's handoff.


## - 2026-11-05 — Level-up healing and local overhead follow view

- Accepted upgrade choices now restore all 5 HP; the English modal explains it.
- Replaced whole-map fitting with a fixed local orthographic view following the
  player, tilted approximately 15 degrees from vertical.
- Scaled player/enemy models and collision footprints to 70%, shared geometry
  values across rendering/simulation/aim, and grounded smaller bodies at Y=1.32.
  B's map and spawn metadata remain unchanged.
- Renamed benchmark workloads for the new camera so old full-map timings are
  not confused with current rendering. Build, 21 tests and render/input smoke
  passed; Windows view/comfort acceptance remains local follow-up work.

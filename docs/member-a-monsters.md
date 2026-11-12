# Member A - mixed enemy encounters

Updated: 2026-11-13 (Australia/Sydney). Built on team baseline `8c1a587`.
The first increment added the approved mixed encounters. The follow-up raises
Golem health, fixes a reproduced corner-routing stall and adds larger Golem EXP
drops. Weapon effects are documented in [hit effects](member-a-effects.md).
B's levels/lighting and C's material/instance attribute contracts are retained.

## Enemy definitions

| Kind | HP | Contact damage | Speed (blocks/s) | EXP |
| --- | --- | --- | --- | --- |
| Zombie | 4 | 1 | 3.3 | 1 |
| Golem | **14** | 2 | 2.1 | 2 |
| Spider | **3** | 1 | 4.5 | 1 |

The Golem has **3.5 times Zombie health**, twice its damage and twice its EXP.
Spider HP is exactly
`4 * 0.75 = 3`; enemy health profiles still use floating point. The player's
current speed remains 5. The existing one-second player damage protection is
shared across all attackers, preventing simultaneous crowd hits from bypassing
invulnerability. A kill awards one orb with the typed EXP value (Golem value 2),
not duplicate drops. The orb retains its source kind. Golem drops use a 0.25
render half-extent versus 0.14 for other drops (about 1.8 times the width).
Attraction/pickup distances and EXP values are unchanged; these are still cube
proxies, not new sphere meshes.

`EnemyTypes.h` is the shared source for kind, profile, geometry and spawn weights.
The Golem is a wider stone biped with amber eyes and a slower walk. The Spider
has a squat purple body/head, eight articulated legs and alternating foot phases.
The existing zombie model/atlas is preserved. The two new 64x64 skin atlases are
original procedural assets, reproducible with `python3 tools/generate_enemy_skins.py`
using only the Python standard library; normal builds use the committed PNGs.

## Encounter curves

All times are **active simulation seconds**, excluding upgrade/benchmark pauses.
Newly joining types ramp linearly over 60 seconds after release. Night opens
with Spiders as its initial population:

| Active time | Day: Zombie/Golem/Spider | Night: Zombie/Golem/Spider |
| --- | --- | --- |
| 0-30 s | 100% / 0% / 0% | 0% / 0% / 100% |
| 60 s | 100% / 0% / 0% | 25% / 0% / 75% |
| 90 s | 85% / 15% / 0% | 50% / 10% / 40% |
| 120-180 s | **70% / 30% / 0%** | **50% / 20% / 30%** |

Day Golems start after 60 s. Night starts with Spiders; Zombies ramp in from
30-90 s and Golems from 60-120 s, finishing at Spider:Zombie:Golem = 3:5:2.
Day never autonomously spawns Spiders. Weighted random selection describes
generation proportions when entries are available; live proportions vary with
kills. Every selected body is tested against every entry. If all entries reject
it, another currently eligible kind can fill the slot. No ineligible type is
introduced as a fallback. Blocked entries can therefore affect the realised mix.

Night attempts a batch every **3 seconds before 30 active seconds** (one third
fewer attempts than the normal two-second interval). At/after 30 seconds it uses
2 seconds, as Day does throughout. The first batch is immediate. Add one to batch
size every 30 seconds, and cap at 24 live enemies. Automatic encounters now require **six
blocks of horizontal clearance between player/enemy body edges**, using the live
player position and selected type's footprint. This replaces the old automatic
four-block centre-distance rule. Every entry/type fallback repeats the safety
check. When all entries are unsafe/blocked, skip the slot and retry at the next
batch without reducing the safe distance. Safety applies equally in Day/Night
and near map edges; it is a distance guarantee, not off-screen spawning.

## Pursuit, collision and crowding

- Each body has its own height, extents and movement speed. Spawn checks,
  navigation, melee reach, projectile hits and player blocking use these values.
- Closest enemies move first, freeing space ahead of followers without changing
  stable IDs or snapshot order. This improves queues behind slow Golems.
- Persistent grid waypoints avoid the old turn-centre oscillation. BFS checks
  both tile occupancy and swept routes between tiles against other bodies.
  Stable per-ID direction order spreads equivalent routes around obstacles.
- A grid turn remains committed until reached or blocked by the world. Temporary
  actor blockage waits on the route before the 0.35-second timed replan; another
  monster entering the route does not clear it every frame. Direct goals still
  follow the live player. Where the player is reachable in open world geometry,
  crowd detours may advance or sidestep, but their first step cannot retreat.
- Voxel route checks now use continuous segment/slab tests against expanded
  blocks, with the same edge tolerance as occupancy. The old 0.1-block sampling
  could miss a tiny corner clip, repeatedly choosing a route that movement
  rejected. World-invalid waypoints are validated before movement. Off-centre
  actors reconnect through their free cell centre only when a static corner
  prevents connection to all neighbouring grid centres. Nearby actors alone
  cannot trigger this re-centring. Recovery never teleports
  an actor or permits wall/actor penetration.
- Direct approach targets stop outside the combined player/enemy footprint.
  Large Golems can approach a player near a wall without requiring the Golem to
  stand at the player's centre. Blocked paths replan; reachable nearer tiles
  serve as fallback when a crowd temporarily seals the final approach.
- Short movement substeps retain block and actor separation. Melee also checks
  line of sight through solid corners. Narrow lanes still queue physically;
  monsters do not phase through one another.
- All types retain B's 0.7 water-speed multiplier. Reset/level switching clears
  typed actors, render poses and the encounter clock while preserving the chosen
  map's Day/Night encounter rules.

## Interfaces for B/C

`Enemy.kind` selects `enemyProfile(kind)`. `spawnEnemy(..., kind)` defaults to
Zombie for existing callers. That explicit API also permits staging any model
for tests and retains its original four-block minimum. Autonomous spawning is
the production path; it additionally enforces `spawnSafetyRadius()` (6), typed
body-edge clearance and level/time restrictions before invoking the staging hook.

Enemy centres differ: Zombie Y=1.32, Golem Y=1.442, Spider Y=1.212. All bases sit
just above the floor at Y=1. Do not apply the old shared Zombie bounds to a new
kind. The existing player-height aim plane still intersects all three bodies.
B's layout metadata remains unchanged; spawn consumers ground the selected body.

`Gameplay::reset(level)` sets encounter rules; parameterless `reset()` retains
them. `ArenaScene` passes its selected level and resets both simulation and poses.
The old `enemySpeed()`/`enemyHalfExtents()` accessors remain Zombie compatibility
helpers; new code should use the type profile.

`ZombieInstance` and `drawZombies` retain their names for C compatibility but now
accept all three kinds. Rendering groups by kind and part: six instanced batches
per biped kind and ten per Spider (head, body and eight legs), independent of
actor count. Instance attributes 3-6 and normal attribute 7 remain unchanged.
Debug geometry/draw counts reflect present kinds. Attack events and existing
walk/facing snapshots drive the new poses. The separate particle instance VAO
adds colour at attribute 8 without changing these model attributes.

## Validation and remaining limits

- Full application CMake Release build passed on Linux/GCC 14.2.
- **18 enemy test cases passed**, covering exact ratios, time curves,
  weighted generation, typed clearance, damage/invulnerability, typed HP,
  rewards, mixed crowd routing, wall-adjacent approach, pause/reset, both real
  maps' entry compatibility, the cap, water slowdown and corner-grazing recovery
  across all three body types, automatic body-edge safety, near-entry rejection,
  no-safe-entry skipping and live-player safety after moving onto an old entry.
  Every navigation movement step checks clearance and speed.
- Two crowd regressions cover 36 side-by-side type/target configurations and a
  narrow-lane follower that waits steadily, then resumes when the player moves.
  Both fail against the previous navigation code: one records eight repeated
  reversals; the other moves a full 0.09 blocks while it should be waiting.
  Both pass now, with per-step clearance, speed and no-overlap checks retained.
- Current suite: **50 cases passed**. The earlier monster-only delivery recorded
  28/34 passing: six tests still asserted obsolete weapon, speed and camera rules.
  The follow-up refreshes those expectations and adds real weapon-unlock,
  independent-cooldown, progression/evolution and particle coverage. Tests are
  neither disabled nor used to revert the team's current weapon design.
- A separate OpenGL harness staged all three models in Day and Night, rendered
  production shaders/lighting and instanced meshes without GL errors, and checked
  level/reset state. Screenshots inspected. Spiders staged in the Day rendering
  fixture are not autonomous Day encounters.
- CLI 240-frame render smoke passed. This is software-render validation, not
  hardware performance evidence or a baseline/instancing comparison.

Manual Windows playtesting, full three-minute balance acceptance and CSE/VLAB
validation remain open. See [validation and replay](member-a-validation.md) for
the current evidence and the exact explanation of the superseded assertions.

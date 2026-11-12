# Member A - mixed enemy encounters

Updated: 2026-11-12 (Australia/Sydney). Built on team baseline `8c1a587`.
This increment implements the approved monster request only. Weapon progression,
weapon hit effects, B's levels/lighting and C's material/instance attribute
contracts are retained.

## Enemy definitions

| Kind | HP | Contact damage | Speed (blocks/s) | EXP |
| --- | --- | --- | --- | --- |
| Zombie | 4 | 1 | 3.3 | 1 |
| Golem | 8 | 2 | 2.1 | 2 |
| Spider | **3.4** | 1 | 4.5 | 1 |

The Golem has exactly twice Zombie health, damage and EXP. Spider HP is exactly
`4 * 0.85`; enemy health is floating point rather than rounded. The player's
current speed remains 5. The existing one-second player damage protection is
shared across all attackers, preventing simultaneous crowd hits from bypassing
invulnerability. A kill awards one orb with the typed EXP value (Golem value 2),
not duplicate drops.

`EnemyTypes.h` is the shared source for kind, profile, geometry and spawn weights.
The Golem is a wider stone biped with amber eyes and a slower walk. The Spider
has a squat purple body/head, eight articulated legs and alternating foot phases.
The existing zombie model/atlas is preserved. The two new 64x64 skin atlases are
original procedural assets, reproducible with `python3 tools/generate_enemy_skins.py`
using only the Python standard library; normal builds use the committed PNGs.

## Encounter curves

All times are **active simulation seconds**, excluding upgrade/benchmark pauses.
Each new type ramps linearly over 60 seconds after its release time:

| Active time | Day: Zombie/Golem/Spider | Night: Zombie/Golem/Spider |
| --- | --- | --- |
| 0-30 s | 100% / 0% / 0% | 100% / 0% / 0% |
| 60 s | 100% / 0% / 0% | 92.5% / 7.5% / 0% |
| 90 s | 85% / 15% / 0% | 72.5% / 15% / 12.5% |
| 120-180 s | **70% / 30% / 0%** | **60% / 15% / 25%** |

Day Golems start after 60 s, Night Golems after 30 s, and Night Spiders after
60 s. Day never autonomously spawns Spiders. Weighted random selection describes
generation proportions when entries are available; live proportions vary with
kills. Every selected body is tested against every entry. If all entries reject
it, another currently eligible kind can fill the slot. No ineligible type is
introduced as a fallback. Blocked entries can therefore affect the realised mix.

Retained team rules: attempt a batch every 2 seconds, add one to batch size every
30 seconds, cap at 24 live enemies, and require at least four blocks of player
separation. No special off-screen spawn system or additional difficulty mode is
introduced in this increment.

## Pursuit, collision and crowding

- Each body has its own height, extents and movement speed. Spawn checks,
  navigation, melee reach, projectile hits and player blocking use these values.
- Closest enemies move first, freeing space ahead of followers without changing
  stable IDs or snapshot order. This improves queues behind slow Golems.
- Persistent grid waypoints avoid the old turn-centre oscillation. BFS checks
  both tile occupancy and swept routes between tiles against other bodies.
  Stable per-ID direction order spreads equivalent routes around obstacles.
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
for tests; autonomous spawning applies the level/time restrictions.

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
walk/facing snapshots drive the new poses; no new weapon effects are added.

## Validation and remaining limits

- Full application CMake Release build passed on Linux/GCC 14.2.
- **12 new enemy test cases passed**, covering exact ratios, time curves,
  weighted generation, typed clearance, damage/invulnerability, fractional HP,
  rewards, mixed crowd routing, wall-adjacent approach, pause/reset, both real
  maps' entry compatibility, the cap and water slowdown.
- CPU suite: **34 cases, 28 passed, 6 failed**. These are the same six baseline
  failures identified before this change: old weapon profile/hit-count/bomb/
  piercing expectations, old 4.3 enemy speed and old 26-high camera assertion.
  They are not hidden, disabled or rewritten as part of this monster-only task.
- A separate OpenGL harness staged all three models in Day and Night, rendered
  production shaders/lighting and instanced meshes without GL errors, and checked
  level/reset state. Screenshots inspected. Spiders staged in the Day rendering
  fixture are not autonomous Day encounters.
- CLI 240-frame render smoke passed. This is software-render validation, not
  hardware performance evidence or a baseline/instancing comparison.

Manual Windows playtesting, full three-minute balance acceptance and CSE/VLAB
validation remain open. Weapon hit-particle completion and the baseline test
refresh remain separate pending tasks as requested.

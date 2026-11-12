# Member A progress - gameplay, enemies and weapon effects

Updated: 2026-11-13 (Australia/Sydney). Current integration builds on team
baseline `8c1a587` and mixed-enemy delivery `e32b7cf`.

## Current scope

ProjectPlan.pdf assigns Member A core gameplay, game state and weapon-hit
particles. A's gameplay systems, all three requested enemy types and weapon-hit
styles are implemented. This does not claim final team submission readiness:
Windows acceptance of this follow-up, CSE/VLAB validation, final balance and
report/video evidence remain team follow-up work.

| Plan milestone | A deliverable | Status |
| --- | --- | --- |
| M1 / Week 7 | Movement, shooting, enemies and collision | Implemented; supplied movement retained |
| M2 / Week 8 | EXP attraction, random upgrades and game state | Implemented; English selection/result menus and full healing |
| M3 / Week 9 | Distinct hit particles and integration evidence | Implemented and Linux-tested; final platform/report acceptance remains |

The latest follow-up raises Golem health to 14, fixes a reproduced corner-grazing
stall, enlarges Golem drops, adds weapon impacts/rotating ground fire and weak
Night fill lights, and refreshes obsolete regression assertions. It preserves
teammate weapon progression, individual cooldowns, layouts, instancing and BRDF/
normal-map interfaces. Exact details live in [mixed enemy encounters](member-a-monsters.md)
and [weapon effects](member-a-effects.md).

The latest playtest revisions use a six-block body-edge safety zone for every
automatic spawn attempt and restore the pre-effects firebomb trail verbatim from
`e32b7cf`. The original ten ground-fire cubes retain their appearance and rise,
with only angular motion sped up to form a vortex. The accepted radial explosion
burst is retained.

The crowd follow-up separates temporary body congestion from static-corner
recovery. Grid turns remain committed; blocked followers wait before replanning,
and open-world crowd routing cannot start with a retreat. Both new crowd
regressions fail on the old code and pass with this change. Accepted particles
retain the `5af6183` implementation.

## Gameplay delivered by A

- Camera-relative eight-direction WASD with normalized diagonals, block sliding
  and actor blocking; player speed is 5 blocks/second. Supplied movement was
  integrated rather than authored anew.
- Mouse-to-world aiming uses logical window coordinates and the real framebuffer
  view-projection on Y=1.32. Holding left mouse fires all owned, ready weapons.
  ImGui input capture suppresses gameplay input.
- Swept projectile collision prevents tunnelling. Wall contacts win collision
  ties. Each projectile keeps its launch-time damage, speed, weapon and evolution.
- Typed spawning uses B's live candidates, a six-block body-edge safety zone,
  body-specific clearance, a 24-live-enemy cap and normally two-second batches
  (1.5 seconds during the first 30 seconds of Night, ramping to two by 60 seconds).
  Batch size gains one every 30 active seconds.
  Unsafe candidates remain rejected during type/entry fallback; a batch is
  skipped when no safe entry exists. The explicit staging API retains its old
  four-block minimum for deterministic fixtures and existing callers.
- Enemy chase uses persistent four-neighbour grid waypoints, continuous swept
  world clearance, live actor separation and front-first movement. Current-cell
  connection and no-progress recovery handle off-centre corner grazing. Narrow
  passages queue steadily; actor blockage does not trigger per-frame re-centring.
  Grid turns stay committed, while direct chase follows the moving player.
  Recovery does not teleport actors through obstacles.
- Kills drop typed EXP cubes; attraction starts within four blocks at seven
  blocks/second, pickup occurs within 0.65 blocks and intentionally ignores cover.
  Golem drops are visibly larger and grant 2 EXP; other drops grant 1.
- Threshold is level times three EXP. Up to two distinct random choices offer
  upgrades/evolution for owned weapons or unlock Laser/Bomb. Selection consumes
  EXP, increases level and restores 5/5 HP. Unlocks add to the arsenal. Gameplay,
  timer and visual effects pause while a choice is pending.
- Player has 5 HP and one-second shared contact-damage protection. Zero HP loses;
  surviving 180 active seconds wins, with loss winning a same-frame tie. Clear
  English VICTORY/DEFEAT menus show statistics and Play again.
- Restart clears actors, drops, progression, timers, render poses and effects,
  returning to the selected map's spawn. Level switching starts a fresh run and
  restarts its Day/Night encounter clock.
- Smaller grounded actors and a fixed local follow camera show the player's
  neighbourhood. Orthographic half-height is 8, with a camera offset of 7
  horizontally and 12 vertically (about 60 degrees above the ground). Zoom stays
  fixed near map edges. This is view clipping, not exploration/fog-of-war.

## Current team weapon rules

The arsenal/progression redesign was supplied by the team in `8c1a587`; A retains
and now regression-tests it. Older Heavy/Rapid and fixed 2/4/1 kill-count entries
in CHANGELOG describe earlier designs and are superseded by these rules.

| Weapon | Base damage | Interval | Speed | Base behaviour |
| --- | --- | --- | --- | --- |
| Bullet | 2 | 0.30 s | 5 | Stops at first enemy/wall |
| Laser | 1 | 0.48 s | 40 | Stops at first enemy; one wall reflection |
| Bomb | 2 | 1.20 s | 8 | Full radial damage within 1.75 blocks |

Bullet stages: speed x2, damage x2, speed x2, interval /2; fifth selection evolves
into three projectiles with two smaller base-damage pellets at +/-30 degrees.
Laser stages: damage x2, interval /2, reflections +1, interval /2; evolution turns
red and pierces before its first bounce, then damages and stops on the next enemy.
Bomb stages: radius x2, speed +5, damage x2, interval x2/3; evolution leaves a
three-second burning area doing 2 damage per second. Blast damage has no falloff,
reaches enemies behind cover within the radius and does not hurt the player.
Bombs also detonate at their three-second lifetime limit.

A's new hit styles are short Bullet debris, cyan/red Laser electricity, radial
Bomb bursts and evolved Bomb orange-red fire particles rotating into ground
vortices. Flight uses the original four lit trail sparks, and ground fire uses
the original ten lit cubes; impact bursts use the emissive instanced batch.
Night adds bounded, weak local fill lights; Day lighting is unchanged. Impact
bursts use independent RNG and a single instanced draw, without modifying combat
or spawn randomness. Original trail/ground-fire cubes retain lit proxy rendering.

## Enemy profiles and encounter timing

| Kind | HP | Damage | Speed | EXP |
| --- | --- | --- | --- | --- |
| Zombie | 4 | 1 | 3.3 | 1 |
| Golem | 14 (3.5x Zombie) | 2 | 2.1 | 2 |
| Spider | 3 (0.75x Zombie) | 1 | 4.5 | 1 |

Day Golems enter after 60 seconds and ramp to Zombie:Golem 70:30 by 120.
Night starts with Spiders; Zombies enter after 30 seconds and Golems after 60.
By 120 seconds the weights are Spider:Zombie:Golem 30:50:20.
Night uses a 1.5-second spawn interval until 30 seconds, then increases it
linearly to two seconds by 60 seconds and holds it there. Each new type ramps linearly for 60
seconds. These are generation weights, not quotas for surviving enemies.
Three distinct hierarchical models, typed geometry and procedural Golem/Spider
skin assets are integrated with C's instanced renderer. All types retain B's
water slowdown. See the monster specification for exact heights and extents.

## Interfaces and team ownership

| Owner | Handoff |
| --- | --- |
| B: maps, collision, lighting | A reads the live `ArenaWorld`, `canOccupy` and layout spawns. Keep metadata clear for typed bodies and call `rebuildWorldGeometry()` after edits. New Night fill light uniforms are presentation-only and preserve B's main light/materials. |
| C: presentation and instancing | Read gameplay snapshots, stable enemy IDs, `stats()`, `weapons()` and `upgradeOptions()`. Use `chooseUpgrade(index)` and scene reset rather than changing phase directly. Preserve pause/heal/reset behaviour when changing UI. |
| C: effects/rendering | `impacts()` adds launch-time evolution and radius to transient contact data. Scene consumes once per simulation update. Particle colour uses attribute 8 on its own VAO; existing matrix attributes 3-6 and normal 7 remain. |
| Team: geometry | `ActorGeometry.h` and `EnemyTypes.h` are authoritative. Player/default Zombie centre is Y=1.32; Golem/Spider differ. Use each profile for navigation, hit bounds, drawing and spawn grounding. |

`shots()`, `hits()`, `attacks()` and `impacts()` clear at the next update/reset;
consume before advancing again, and do not retain references. `enemies()`,
`bullets()`, `orbs()`, `explosions()` and `burningAreas()` are read-only snapshots.
Existing APIs are retained. `ExperienceOrb::sourceKind` adds type-driven pickup
presentation without changing attraction/reward rules. `ZombieInstance` and
`drawZombies` retain their compatibility names while supporting all three types.

B's Day/Night maps, water, player light, normal maps and material shading are
teammate work. C's HUD/card presentation, model/animation integration and
instancing/BRDF contributions are not claimed as new A work. The supplied start
screen, textured player, WASD, ImGui lifecycle and benchmark also predate A.

## Verification and remaining acceptance

Current Linux C++17 Release application build and all **50 regression cases**
pass. Coverage includes three typed bodies, encounter ramps, corner recovery,
automatic spawn safety, real unlocks and independent cooldowns, all weapon upgrades/evolutions, particles,
lights, reset and pause. Six earlier failures came from tests retaining obsolete
rules, detailed in [validation and replay](member-a-validation.md). No test is
disabled and no weapon behaviour was reverted to satisfy an old assertion.

Production-shader Day/Night render checks cover impacts, flame trails and rotating ground fire, GL state,
real impacts, upgrade pause and reset. Pixel comparisons confirm Night fill is
local and does not change Day or a distant control region. The CLI 240-frame
software-render smoke is a runtime check, not comparative hardware evidence.

Member A previously accepted gameplay after Windows/Visual Studio iterations;
that feedback does not constitute Windows validation of this follow-up. Remaining
work is to replay the current build on Windows and CSE/VLAB, accept full-run
balance, and capture final report/video and matched benchmark evidence with the
team. Build steps and replay actions are in the linked validation guide.

## Delivery history

| Commit | A delivery |
| --- | --- |
| `5cb86d4` | Combat prerequisites, EXP/selection/state integration, tests and handoff |
| `fdd6266` | C++17-compatible initialization for the reported MSVC C7555 failure |
| `ce52a0b` | Reliable centred upgrade selection and initial weapon differentiation |
| `aa53b10` | Balance/Laser/Bomb, crowd routing and result/restart menus |
| `91e0e6f` | Full healing, smaller grounded actors and local follow view |
| `e32b7cf` | Typed Zombie/Golem/Spider encounters, models/skins, curves and navigation |

The present follow-up supplies the Golem rebalance, corner fix, hit effects,
Night glow, larger drops and refreshed regression coverage described above.

## Interactive capture range

[report/member-a](../report/member-a/README.md) now contains a playable stationary,
invulnerable target range, replacing the earlier figure/UI replay pages. It uses
real ArenaScene movement/collision/effects with free weapon levels, SPACE time
stop, N stepping and F auto-stop on contact. Training-only rules require explicit
opt-in; normal Craft gameplay is unchanged. The report/source reference files
remain, with capture instructions updated for the new range.

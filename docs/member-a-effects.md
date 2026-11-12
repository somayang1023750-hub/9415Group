# Member A - weapon impacts and Night fill lights

Updated: 2026-11-12 (Australia/Sydney). Follow-up to mixed encounters in `e32b7cf`.

## Visible behaviour

| Source | Effect | Lifetime |
| --- | --- | --- |
| Bullet impact | Short warm debris burst with gravity | 0.16-0.28 s |
| Base Laser contact/reflection | Five branching cyan electrical arcs | 0.12-0.22 s |
| Evolved Laser contact/reflection | Same electrical shape in red | 0.12-0.22 s |
| Bomb detonation | Particles expand radially from the blast centre | 0.38-0.65 s |
| Evolved Bomb detonation/residue | Red burst, followed by three rotating red ground-spiral arms | Ground vortex follows the 3 s burning area |

The existing evolved-Bomb flight trail and short explosion-radius ring remain.
The ground vortex replaces the previous orange ground ring/rising sparks for
burning areas. Effect shape, colour and motion distinguish the weapons without
introducing separate weapon models. These visuals do not change damage, blast
radius, upgrades, reflection rules, unlocks or independent cooldowns.

At Night, impacts briefly cast weak warm/cyan/red local fill light. A burning
area contributes a faint red light until it expires, fading over its final
0.35 seconds. Impact flashes last 0.18 seconds (Bullet/Laser) or 0.5 seconds
(Bomb). Light radius is 1.3 blocks for Bullet, 1.6 for Laser, and at most four
for Bomb/residue. The coloured particles also remain visible without ambient
illumination. Day receives no additional scene lighting.

This is small unshadowed fill lighting with distance falloff, not a new shadow
system or a global Night brightness increase. B's player light, material and
normal-map controls remain active. Distant surfaces outside the effect radius
receive no added light.

## Interfaces for B/C

- `Gameplay::impacts()` is a frame-local presentation stream containing contact
  position/direction, weapon, launch-time evolution flag and Bomb radius. It
  clears at each simulation update/reset. Bullet/Laser enemy hits, wall contacts
  and Laser reflections emit impacts. A Bomb emits one detonation event, rather
  than a separate burst for every victim. Burning-area damage ticks do not emit
  repeated bursts. Existing `shots()`, `hits()` and `attacks()` contracts remain.
- `ParticleEffects` owns CPU particle motion, independent seeded visual RNG and
  derived local lights. It reads impacts and `burningAreas()` without changing
  gameplay state. The maximum pools are 2,048 live burst segments, 4,096 total
  particle visuals including vortices, and eight uploaded effect lights.
- `ArenaScene` consumes events exactly once when simulation advances. Upgrade,
  result and benchmark pauses freeze particles/lights. Restart and level changes
  clear all effect buffers. C can read `particleCount()` for diagnostics.
- `ArenaRenderer::drawParticles()` uses one instanced draw with a dedicated VAO.
  Existing model-matrix attributes 3-6 and normal attribute 7 are preserved;
  attribute 8 holds per-instance RGBA. The `uParticlePass` shader branch renders
  fading coloured segments without material lighting. Depth testing remains
  active, depth writes are disabled during the pass, and blending/depth-write
  state is restored afterwards.
- `setEffectLights()` uploads at most eight position/colour/radius entries.
  `uEffectLightCount` is zero in Day. Shader fill is added to both BRDF surfaces
  and lit actor/masonry surfaces, with a clamped total contribution. B can tune
  falloff/strength independently of gameplay. This adds no extra terrain draw.
- `ExperienceOrb::sourceKind` permits type-based pickup visuals. Golem EXP cubes
  have a 0.25 half-extent versus 0.14 for Zombie/Spider (about 1.8x width), still
  grant 2 EXP and retain the original attraction/pickup distances.

## Verification

- The full C++17 application builds in Linux Release; all 43 current tests pass.
  Seven particle cases cover colour/expiry, radial motion, ground lifetime,
  pause/reset, bounded pools, transient real wall events, visual RNG isolation
  and bounded fading lights. Enemy tests cover Golem HP and typed orb rewards.
- An OpenGL/Xvfb/Mesa harness renders all five effect styles using production
  shaders in Day and Night. No GL errors were observed. It also checks blend/
  depth-write restoration and real scene impacts, upgrade pause, restart and
  level-reset clearing. The harness is a temporary diagnostic, not a runtime
  dependency or committed test target.
- Pixel comparison of the same frame with fill lights enabled/disabled shows
  identical Day surfaces; Night differs near the impacts. The distant control
  region is identical in both modes. This demonstrates local lighting, not
  hardware performance or Windows acceptance.

Replay on Windows: wound the player, kill a Golem, inspect its larger drop,
collect EXP, unlock/upgrade the weapons, and shoot enemies and cover in Night.
Verify cyan/red electrical contacts, outward blasts, the three-second red vortex
and small local light pools. Pause on an upgrade, then restart or switch level;
effects should freeze during the pause and disappear on a reset.

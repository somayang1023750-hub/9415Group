# Interactive training range (replacement showcase)

The old Figure 1–4 replay pages and canned UI fixtures have been removed from
MemberAShowcase. This executable is now a playable capture range using the real
ArenaScene, player movement, mouse projection, projectile collision, models,
weapon profiles and particle rendering. The normal Craft game remains normal.

## Windows

Pull main, then run `report\member-a\build_windows.cmd` from the Visual Studio
Developer Command Prompt. Run `build-report\Debug\MemberAShowcase.exe` afterwards
(or `build-report\MemberAShowcase.exe` with Ninja/single-config builds).
You can also set `-DCRAFT_BUILD_MEMBER_A_SHOWCASE=ON` in Visual Studio CMake
settings and select MemberAShowcase as the startup target. The target name and
build script are retained so an existing build-report folder can be rebuilt.

## Range and controls

Five real enemy models stand still beside cover or the outer walls. They remain
solid and can be hit indefinitely, but do not lose health, die, attack or drop
EXP. There are no automatic waves and no survival countdown. Walk to outer-wall
targets using the normal following camera.

- WASD: move. Mouse: aim. Hold left mouse: fire all enabled weapons.
- SPACE: stop/resume time instantly. No simulation update runs while stopped;
  projectiles, impacts, fire, animation phases and cooldowns retain their state.
- N: while stopped, advance exactly 1/60 second.
- F: toggle automatic time stop on a real collision/impact, including walls.
- 1 / 2 / 3: fire only Bullet / Laser / Bomb, preserving their chosen upgrades.
- U: upgrade the selected weapon; after four stages the next press evolves it.
- E: immediately set the selected weapon to stage four and evolved.
- The panel also sets owned weapons, stages 0–4, evolution and Night lighting.
  Changes do not rewrite already-fired projectiles or clear frozen effects.
- H: hide/show panel. F12: save PNG to `member-a-captures/` in the launch folder.
- Reset range: deliberately clears actors/projectiles/effects and resets loadout.
- ESC: exit.

For hit screenshots, enable auto-stop (F), shoot a target or wall, then use N to
advance the frozen burst a few frames. For fire rotation, enable/evolve Bomb and
shoot, then stop at different ages. Real in-game effect sizes, colours, lifetime,
ricochet/piercing and fire damage rules are reused. Only target damage/movement,
spawn waves and survival end conditions are disabled in this explicit range.

The Word and numbered source excerpts remain reference material. Figure 2's
upgrade/victory/defeat screenshots should now be taken from the normal Craft game;
the range intentionally has no old UI demonstration pages.
`py -3 report/member-a/export_code.py` refreshes excerpts after source changes.
`MemberAShowcase --smoke` exercises real hits, invulnerability, frozen collision
and fire snapshots, resume and PNG capture; it is not a performance benchmark.

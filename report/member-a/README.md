# Interactive training range (replacement showcase)

The old Figure 1–4 replay pages and canned UI fixtures are replaced by
MemberATrainingRange v2, a playable capture range using the real
ArenaScene, player movement, mouse projection, projectile collision, models,
weapon profiles and particle rendering. The normal Craft game remains normal.

## Windows

Pull main, close any old demo window, then run
`report\member-a\build_windows.cmd` from the Visual Studio Developer Command
Prompt. **It builds and launches the new program automatically.** Do not manually
pick an old MemberAShowcase.exe afterwards.

The script uses a separate `build-training-range` folder, reads CMake's exact
configuration-specific target path, and displays SOURCE_ROOT, SOURCE_COMMIT,
LAUNCHING_EXACT_FILE and the executable timestamp. Build/launch failures stop
without falling back to any existing executable.

The new executable is **MemberATrainingRange.exe**. Its window and panel say
**TRAINING RANGE v2**, and the panel shows the compilation date/time. It has no
Figure 1–4 buttons. With Visual Studio, the usual output is
`build-training-range/Debug/MemberATrainingRange.exe`; Ninja typically uses the
build-directory root. The script obtains this path from CMake, not a guess.

The CMake build target/option retain their existing names for compatibility:
`MemberAShowcase` and `CRAFT_BUILD_MEMBER_A_SHOWCASE=ON`. If using Visual Studio's
own folder build, select that target; its executable name is MemberATrainingRange.

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
`MemberATrainingRange --smoke` exercises real hits, invulnerability, frozen collision
and fire snapshots, resume and PNG capture; it is not a performance benchmark.

## Gameplay rebalance note (2026-11-13)

The editable Word snapshot retains its earlier gameplay baseline to avoid
overwriting a contributor's in-progress report. For your final report, update
Spider HP to 3 (0.75x Zombie), and Night's sequence to Spider at start, Zombie
after 30 s, Golem after 60 s. The final weights at 120 s are Spider:Zombie:Golem
30:50:20; early Night's batch interval is 3 s before 30 s, then 2 s. See the
current [enemy specification](../../docs/member-a-monsters.md).

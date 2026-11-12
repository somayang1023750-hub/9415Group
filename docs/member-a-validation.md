# Member A validation and replay guide

Updated: 2026-11-12 (Australia/Sydney). Current follow-up builds on `e32b7cf`.

## Current evidence

| Check | Recorded outcome | Limit |
| --- | --- | --- |
| Full C++17 application build | Passed, Linux Release / GCC 14.2 | Not a Windows or CSE result |
| Current regression suite | 46 cases passed: 17 gameplay, 15 enemy, eight particle, five world, one math | CPU checks do not replace visual/balance acceptance |
| Automatic spawn safety | Eight-block body-edge exclusion tested in Day/Night, including unsafe fallbacks, no safe entries and moving onto an old entry | Explicit fixture staging retains the old four-block minimum |
| Corner-grazing regression | All three kinds and multiple edge offsets reach the target; every step checks clearance and speed | Crowded lanes may still queue physically |
| Weapon integration | Real unlocks, independent cooldowns, four upgrades and evolution per weapon passed | Current team rules retained |
| Particle/light integration | Production OpenGL shaders render impacts, fire trails and rotating ground fire in Day/Night without GL errors; real impacts, pause/reset and blend/depth-write restoration passed | Temporary Xvfb/Mesa harness, not Windows hardware |
| Local lighting comparison | Day identical with/without effect fill; Night changes near impacts; distant control region identical | Pixel comparison, not shadow or performance evidence |
| CLI smoke | 240 measured frames completed | Static software-render smoke, not an instancing comparison |
| Windows/CSE and full-run acceptance | Current follow-up still needs replay | Earlier Windows feedback applies to the earlier gameplay version |

See [mixed enemies](member-a-monsters.md) and [weapon effects](member-a-effects.md)
for exact contracts. The new tests are part of the normal `test_*.cpp` discovery;
none is disabled or expected to fail.

## What the six earlier failing cases meant

Those tests predated the weapon redesign in `8c1a587`. Six failing **test cases**
could produce more than six failed assertions because Catch2 replays sections.
They did not demonstrate a runtime deadlock or a broken level switch.

| Old case | Obsolete expectation | Current rule / refreshed check |
| --- | --- | --- |
| Weapon profiles | Laser faster-firing than Bullet by over 2x; Bomb stronger/slower projectile than Bullet | Exact current base/staged profiles, including Laser interval 0.48 s, Bullet 0.30 s and Bomb base damage 2 |
| Laser penetration/reflection | Base Laser damages every aligned enemy; exactly one total projectile after Laser selection | Base Laser stops at the first enemy; Bullet remains owned. Reflection tests identify Laser projectiles explicitly. Evolved piercing state is checked after real progression. |
| Bomb radial kills | Two full-health Zombies die from one base Bomb | Base Bomb deals 2 damage within 1.75 blocks; inside/outside health and cover-triggered blast are checked |
| Fixed hit counts after selection | Selection replaces the only weapon; fixed two/four/one kills regardless of upgrades | Unlocks add weapons. Real arsenal/cooldown tests check concurrent shots; progression tests check each weapon's actual stages/evolution. |
| Enemy speed | 0.43-block displacement over 0.1 s (speed 4.3) | Current Zombie speed is 3.3, giving 0.33 blocks |
| Camera angle | Offset ratio 7/26 | Current local view uses 7/12 |

The test refresh preserves current gameplay and strengthens coverage. The
reported corner stall was a separate reproducible navigation issue and is fixed
with continuous route checks and safe re-centring/replanning.

## Earlier baseline evidence (2026-11-05)

The following records describe baseline `91e0e6f`, before the latest team weapon
redesign, mixed enemies and particle follow-up. They are retained as historical
UI/input evidence rather than current weapon assertions.

| Check | Recorded outcome | Limit |
| --- | --- | --- |
| Linux C++17 configure/build | Passed, GCC 14.2 / CMake 3.31 | Not a Windows or CSE build result |
| Existing CTest suite | 21/21 cases passed | 16 A gameplay cases + four B world cases + one starter math case |
| Weapon/simulation tests | Piercing, one reflection, AOE, 2/4/1 hits, crowd routing, EXP, healing and state transitions passed | CPU regression coverage, not final visual/balance acceptance |
| Camera/geometry tests | Local clipping/follow/aim across aspect ratios and grounded 70% actor extents passed | Does not replace physical-device visual checks |
| ImGui interaction harness | Selection and outcome menus fit at 640x480, 1280x720 and 1920x1080; resume/restart requests passed | Temporary local harness, not a committed CI suite |
| OpenGL runtime | Xvfb/Mesa software-render smoke and 240-frame benchmark completed, exit 0 | Not hardware-GPU comparative performance evidence |
| Input/restart smoke | Enter/WASD/click/Escape and defeat/Play again exercised; screenshots inspected | Automated input, not exhaustive manual playtesting |
| Windows user feedback | Member A ran the project in Visual Studio, iterated on reported issues and accepted current gameplay in chat | Agent did not independently run Windows CI or receive a complete Windows test log |
| CSE/VLAB and final team integration | Outstanding | Must be checked before submission |

The local ImGui harness and smoke
screenshots were diagnostic tools, not assets required to build the repository.

## Build and run on the existing Windows checkout

No new clone is needed. In Visual Studio, open the repository folder containing
CMakeLists.txt, keep `x64-Debug`, wait for CMake configuration, then:

1. Build > Build All (Ctrl+Shift+B).
2. Select `Craft.exe` / `Craft` as the startup item.
3. Run without debugging (Ctrl+F5).
4. Open Test Explorer and run the discovered CMake/CTest tests if available.

Git update is `git pull origin main` from the checkout. Preserve local changes if
Git reports a conflict; do not delete/reset them just to update.

For an explicit Visual Studio 2022 terminal build, use Developer PowerShell with
Git, CMake and Python/Jinja2 available. This is an alternative to the existing
Visual Studio-managed `out` build; do not mix generators in that build directory.

```powershell
py -3 -m pip install Jinja2
$ProjectPython = py -3 -c "import sys; print(sys.executable)"
cmake -S . -B build-windows -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON "-DPython_EXECUTABLE=$ProjectPython"
cmake --build build-windows --config Debug --parallel 4
ctest --test-dir build-windows -C Debug --output-on-failure
.\build-windows\Debug\Craft.exe
```

The earlier MSVC C7555 failure was fixed in `fdd6266`; the project remains C++17.
The terminal sequence above is guidance for this repository, not an independently
executed Windows validation result. Actual output paths vary with the generator.

## Repeatable manual acceptance sequence

These are expected behaviours to replay after integration changes, not claims
that every row was independently executed on every platform.

| Step | Action | Expected result |
| --- | --- | --- |
| Start | Launch normally, press Enter | Title fades into gameplay |
| Movement | Walk diagonally, along cover and around a corner | Normalized speed; walls and actors block passage |
| Camera | Walk across the arena and towards its edge | Player stays centred, local zoom stays fixed, distant areas are off screen |
| Enemy chase | Kite all three kinds around cover, narrow gaps and corners | Typed speeds/bounds apply; enemies replan without permanent corner stalls or wall penetration; crowded lanes may queue |
| Mixed encounters | Play Day beyond 120 s and Night beyond 120 s | Day ramps to 70:30 Zombie:Golem; Night to 60:15:25 Zombie:Golem:Spider; time excludes pauses |
| Safe spawning | Stand on/near a map entry and cross another entry before the next batch | New enemies appear outside the eight-block body-edge safe zone; no unsafe forced spawn when entries are blocked |
| Bullet | Hold the left mouse button toward a fresh red enemy | Fires at its own cooldown; two base hits kill; cover stops the projectile |
| EXP | Approach a green drop | Attraction starts within four blocks, pickup adds EXP |
| Upgrade | Reach the threshold, preferably while wounded | Clear English paused modal; up to two random upgrade, evolution or weapon-unlock choices; timer/enemies stop |
| Resume/heal | Select an offered card | HP becomes 5/5, EXP is consumed, level increases and gameplay resumes; unlocked weapons are added to the arsenal |
| Laser | Unlock Laser; align enemies and shoot near cover | Base laser stops at first enemy and reflects once; evolved red laser pierces until first bounce, then damages and stops on the next enemy |
| Bomb | Unlock Bomb; aim at enemies or a nearby wall | Base shot deals 2 damage in a 1.75-block radius; at evolution red firebomb leaves sparks and its burning area lasts 3 seconds, dealing 2 damage each second |
| Golem reward | Defeat a Golem, then compare its drop to a Zombie's | Golem has 14 HP; its drop is about 1.8x the width and grants 2 EXP |
| Hit styles | Hit enemies and cover with all weapons | Short Bullet debris; cyan Laser electricity, red after evolution; outward Bomb burst followed by persistent ground fire; evolved Bomb uses its original four trail sparks and ten ground-fire cubes with added orbital speed |
| Night glow | Repeat impacts in Night, then pause/reset/switch level | Small coloured fill lights near impacts/residue; distant Night stays dark; pause freezes effects and resets clear them |
| Defeat | Allow contact damage until HP reaches zero | Centred DEFEAT modal with run statistics and Play again |
| Victory | Survive 180 simulation seconds | Centred VICTORY modal; upgrade pauses do not consume survival time |
| Restart | Click Play again or Restart current configuration | HP/time/level/EXP/kills/actors reset, player returns to current-map spawn |
| Exit | Press Escape | Application closes |

Each owned weapon has an independent cooldown, and holding the left mouse button
fires all ready weapons. Upgrade choices are random, so a desired weapon or
upgrade may require another level or another run. Base values are Bullet damage
2/speed 5/interval 0.30 seconds, Laser damage 1/speed 40/interval 0.48 seconds,
and Bomb damage 2/speed 8/interval 1.20 seconds/radius 1.75. Own bombs cause no
player damage; their full-radius damage reaches enemies behind cover.
Day/Night selection is wired to B's real layouts and restarts the run.

## Benchmark replay

PowerShell, after the explicit build above:

```powershell
$env:CRAFT_BENCHMARK = "1"
.\build-windows\Debug\Craft.exe
Remove-Item Env:CRAFT_BENCHMARK
```

CLI run skips the title, freezes simulation, performs 60 warm-up and 240 measured
frames, prints `sample_count=240`, and exits. Unset CRAFT_BENCHMARK afterwards so
the next normal launch is playable. Workloads are:

- `milestone2-local-follow-arena`: static command-line scene.
- `milestone2-local-follow-gameplay-frozen`: frozen in-game actor snapshot.

The local camera and actor proportions differ from earlier full-map runs.
Do not treat these timings as a baseline-versus-optimization comparison. Future
C benchmarks need matching camera, resolution, actor snapshot and lighting.

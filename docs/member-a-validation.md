# Member A validation and replay guide

Updated: 2026-11-05 (Australia/Sydney). Gameplay code baseline: `91e0e6f`.

## Evidence recorded so far

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

This documentation update records existing verification; no application code was
changed or new platform validation implied. The local ImGui harness and smoke
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
| Enemy chase | Wait, move around cover, then change direction | Enemies follow at 4.3 versus player 5 blocks/s; crowded lanes reroute or queue without overlap |
| Balance | Aim and click at a fresh red enemy | Two hits kill; cover stops the projectile |
| EXP | Approach a green drop | Attraction starts within four blocks, pickup adds EXP |
| Upgrade | Reach the threshold, preferably while wounded | Clear English paused modal; two distinct random weapon choices; timer/enemies stop |
| Resume/heal | Select an offered weapon | HP becomes 5/5, EXP is consumed, level increases and gameplay resumes |
| Laser | Select Laser when offered; align multiple enemies and shoot near cover | One shot can hit multiple enemies once each, reflects at the first wall, stops at the second; four separate hits kill a fresh enemy |
| Bomb | Select Bomb; aim at enemies or a nearby wall | Slow shot explodes on impact, orange radius ring appears; enemies within 3.5 blocks take full lethal damage, outside enemies survive |
| Defeat | Allow contact damage until HP reaches zero | Centred DEFEAT modal with run statistics and Play again |
| Victory | Survive 180 simulation seconds | Centred VICTORY modal; upgrade pauses do not consume survival time |
| Restart | Click Play again or Restart current configuration | HP/time/level/EXP/kills/actors reset, player returns to current-map spawn |
| Exit | Press Escape | Application closes |

Holding the mouse is not automatic fire. Weapon choices are random, so a desired
weapon may require another level or another run. Damage stays fixed at every
level: Balance 2, Laser 1, Bomb 4 against 4-HP enemies. Own bombs cause no player
damage; their full-radius damage intentionally reaches enemies behind cover.
Day/Night buttons remain disabled until B's layouts and level loading are wired.

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

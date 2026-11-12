# Interactive training range verification

This replaces the earlier staged figure/UI replay executable. Normal Craft never
enables Gameplay's explicit range option. The range opts in to stationary,
invulnerable targets, disabled automatic spawning and disabled survival timeout.
Its weapon controls use normal profiles and do not mutate existing projectiles.

CPU coverage verifies opt-in gating, actual impact/fire events, fixed enemy
positions/health, no kills/EXP/spawn waves or countdown, reset and normal defaults.
OpenGL smoke exercises actual ArenaScene collision/fire, time stop and resume,
frozen projectile position/lifetime and burning-area ages, PNG capture and GL
error checks. Windows requires rebuilding the same MemberAShowcase target.

Full Linux C++17 Release builds of Craft and MemberAShowcase passed, with all
49 CTest cases passing. The 240-frame Xvfb/Mesa range smoke verified real impacts,
unchanged target position/health, frozen projectile lifetimes/fire ages, resume,
PNG output and GL error checks. The visible world region also remains identical
between frozen fire captures (the control checkbox updates on the next UI frame).

## Unambiguous Windows launch (v2)

The executable output is now MemberATrainingRange, while the compatible CMake
build target remains MemberAShowcase. CMake generates a per-configuration file
containing the actual target path; the Windows script reads Debug's manifest
after a successful build and launches that exact path from build-training-range.
Failures never launch an older fallback. The v2 window/panel and compile stamp
identify the binary. Manifest resolution and the renamed program are verified
on Linux; execution of the Windows CMD script itself still needs Windows replay.

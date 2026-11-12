# Member A report capture workspace

This is a **staged demonstration**, not a gameplay/performance benchmark. It
reuses production `ArenaRenderer`, enemy atlases, `ParticleEffects`, GLSL shaders
and `drawGameplayUi`. No alternate particle sizes, colours, lifetimes or enemy
models are implemented here. Only placement, camera and replay timing are staged.
The original four-spark trail is shared through `drawFirebombTrail`, also called
by `ArenaScene`; the extraction preserves its original formulas and rendering.

## Windows / existing Visual Studio checkout

1. Pull main. You can run `report/member-a/build_windows.cmd` from your existing
   Developer Command Prompt to build the separate target. In Visual Studio CMake settings, add the CMake argument
   `-DCRAFT_BUILD_MEMBER_A_SHOWCASE=ON`, then configure and Build All.
2. Choose **MemberAShowcase.exe** as startup target and run with Ctrl+F5.
3. Alternatively, Developer PowerShell from the repository root:

```powershell
cmake -S . -B build-report -DCRAFT_BUILD_MEMBER_A_SHOWCASE=ON
cmake --build build-report --config Debug --target MemberAShowcase --parallel 4
.\build-report\Debug\MemberAShowcase.exe
```

With Ninja/single-config builds the executable is `build-report/MemberAShowcase.exe`.
Keep the generator/Python/dependencies that already work for your normal build.
Do not mix CMake generators in an existing build folder.

## Figures

- **Figure 1:** three actual textured models side by side, at original relative
  sizes. Adjust Model yaw to inspect the skins. Zombie/player foundations are C's
  work; A added Golem/Spider gameplay and their model/skin integration.
- **Figure 2:** buttons stage the real Upgrade, Victory and Defeat phases using
  gameplay updates/kills/EXP. The same production UI draws the modals. Click
  Figure 2 before selecting the UI state; use its controls to prepare another
  state after dismissing one. These are clearly demonstration fixtures.
- **Figure 3:** actual Bullet debris and cyan/red Laser impacts. These are brief;
  `Freeze hit for Figure 3`, Pause, Step 0.02 s and F12 help capture them. The replay cycle is 4.2 seconds.
- **Figure 4:** original flight trail, then the real outward blast and original
  ten ground-fire particles rotating/rising for the normal three-second burn.
  Pause at different ages for paired screenshots. Night uses production lights.
- **Figure 5:** use the real AI code excerpts below; for behaviour evidence use
  the normal game or the existing crowd regression tests, not staged dummy AI.

F1–F4 switch pages even while a modal is open. U/V/D prepare the real
upgrade/victory/defeat UI (use these keys if the modal covers the buttons).
H hides the controls; F12 saves the current framebuffer as a PNG under
`member-a-captures/` in the launch working directory. Press H again to show the
controls. Escape exits. Do not use this staged tool's FPS as performance evidence.

```powershell
py -3 report/member-a/export_code.py
```

`code/` contains numbered excerpts of the actual source for Figures 3–5. They
are documentation snapshots and are not compiled. Re-export after changing the
production implementation. The Word report and Chinese screenshot checklist are
in this folder. Original teammate sections are retained; only A's text and the
explicitly requested particle section are rewritten. Figure labels are A1–A5
in the report to avoid colliding with the teammate's existing Figures 1 and 2.

For a quick automated OpenGL check: `MemberAShowcase --smoke` renders all four
pages, saves captures, checks GL errors and exits (a display is required).

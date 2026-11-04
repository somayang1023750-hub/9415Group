# Craft 3D OpenGL Starter for Group Assignment - COMP3421/COMP9415 26T3

## Overview

The codebase is the starter code for the COMP3421/COMP9415 26T3 Computer Graphics group assignment. The project plan describes a 2.5D top-down survival arena shooter. You can find the assignment specification in [Group Assignment Specification repo](https://gitlab.cse.unsw.edu.au/coursework/COMP3421/26T3/group-assignment-spec).

The current gameplay build opens an OpenGL 3.3 window, shows a start screen, and then renders a 32 × 32 × 32 block volume with a bounded arena floor, low perimeter walls, eight cover groups, a textured six-part player, and a fixed diagonal follow camera. WASD movement is relative to the camera and stops at solid blocks; the player's arms and legs swing during movement. The project does **not** yet provide:

- M3 weapon-hit particle effects or final gameplay balance.
- Final enemy models/animation, glowing EXP sphere visuals, layered player-skin overlays, or a full lighting model.
- Chunk streaming, indexed meshing, or instanced rendering.

Member A's M2 increment adds one enemy type, click-to-shoot projectiles, EXP
attraction, random weapon offers, HP/countdown/outcomes and restart. Enemies,
bullets and EXP use temporary coloured cube proxies; the gameplay panel is an
integration UI for Member C to replace. B's current map remains the M1 layout;
Day/Night switching is pending. See [Member A progress](docs/member-a-progress.md).

Implement the required features described in the [assignment specification](https://gitlab.cse.unsw.edu.au/coursework/COMP3421/26T3/group-assignment-spec).

**IMPORTANT**: You should at least make sure your codabase could be compiled and ran successfully on the CSE machines (VLAB). When we marking your submission, we will test your submission on the CSE machines.

## What is provided

- A C++ CMake project using OpenGL 3.3 core, GLFW, GLAD 2 (on Windows/Linux) and GLM.
- A 32 × 32 × 32 editable block grid with a combined exposed-face arena mesh.
- A Minecraft-style player model with a textured head, torso, arms, and legs; the limbs swing during movement.
- A fixed diagonal orthographic camera that follows the player and keeps the arena framed.
- A start screen titled "Voxel Arena" with a pulsing "PRESS THE ENTER KEY" prompt (see [Start screen](#start-screen)).
- Reusable 2D helpers: `Texture` (loads a PNG with stb_image into an OpenGL texture) and `TextRenderer` (draws bitmap-font text and solid rectangles in screen pixels using `assets/textures/font.png`). You can reuse them for your own HUD, such as a  hotbar.
- One compact Dear ImGui window for arena controls and the course-provided 240-frame benchmark; extend CustomUi with gameplay-specific debug controls.

### Optional (For students who have completed COMP6771 Advanced C++ Programming):

- CTest and Catch2 configuration for optional software testing (It will not be assessed, but students who have completed COMP6771 may find them useful as well).

The arena, player, and renderer are starting implementations that can be extended or replaced for the assignment.

## Project layout

```text
assets/textures/    Textures you need to load and use for your assignment, you need to add your charactor skin texture to this folder. NameMC is a good website to find skins you like.
assets/shaders/     Minimal GLSL shaders, plus the text and start screen shaders
src/app/            ArenaScene, FollowCamera, PlayerController, AppUi, and benchmark timer
src/core/           Asset lookup
src/graphics/       ArenaRenderer, window, shader, texture, and text renderer
src/world/          32 x 32 x 32 block grid and collision queries
tests/              Optional Catch2 self-tests
```

Implementation files under the existing source directories are automatically
collected by `src/CMakeLists.txt`. Use
`craft::core::assetPath(...)` in `main.cpp` to add GLSL shaders. 

## Arena starter and teammate interfaces

Press **Enter** on the start screen, then use **WASD** to move. Movement follows the diagonal camera's screen directions, diagonal input is normalized, and the player slides along arena walls and obstacles.

- `world/ArenaWorld` owns the 32 × 32 × 32 block grid. `blockAt`, `isSolid`, `canOccupy`, and `setBlock` are the map and collision hooks for world generation and gameplay.
- `app/PlayerController` owns the player's position, facing direction, move speed, and movement update. Its `MovementInput` can be filled by keyboard, replay, or AI code.
- `app/FollowCamera` provides camera-relative forward/right vectors and the view-projection matrix. Its diagonal angle stays fixed while its target follows the player.
- `graphics/ArenaRenderer` builds one mesh from exposed block faces and renders the player's six textured body parts. `assets/textures/player.png` uses the classic 64 × 64 skin UV layout. Call `ArenaScene::rebuildWorldGeometry()` after editing the grid.
- `app/ArenaScene` joins the world, player, camera, and renderer. It exposes `world()`, `player()`, and `camera()` for enemy, pickup, HUD, and game-state integration.

The current grid uses X/Z for the arena plane and Y for height. The floor is at Y=0, and the player's feet rest on top of it.

## Member B - Milestone 1 map foundation

The default map is now defined by `world/ArenaLayout`, independently of OpenGL.
It has four corner cover groups, four side barriers, a clear central spawn area,
and connected routes between the centre and the perimeter. Cover and boundary
walls remain two blocks tall to fit the existing camera.

- `makeMilestoneOneLayout()` supplies the current deterministic layout.
- `ArenaWorld(layout)` builds the floor, boundary, and cover, and rejects invalid
  obstacle dimensions or blocked spawn positions.
- `world.playerSpawn()` is the initial player centre used by `ArenaScene`.
- `world.enemySpawnCandidates()` supplies eight entry candidates for Member A.
  These are not an enemy spawner: A still checks enemy dimensions, current
  occupancy, and distance from the player before choosing a candidate.
- Existing `setBlock`, `isSolid`, `canOccupy`, and mesh-rebuild interfaces remain
  available. Spawn metadata describes the original layout; edits can invalidate
  it, so callers must recheck collision after editing the map.

This completes only B's first map increment. Day/Night layouts, UVs, materials,
and lighting belong to Milestone 2; normal mapping belongs to Milestone 3.
A's subsequent M2 increment now supplies enemies and shooting on this map.
See [Member B progress and handoff](docs/member-b-progress.md) for verification
and a GitLab-ready progress summary.

## Member A - M2 gameplay and progression

After Enter, use WASD and click towards red enemy cubes to shoot. Holding the
mouse button does not auto-fire. Kills drop green EXP cubes; approaching attracts
and collects them. Level-up pauses gameplay and opens a centred English selection dialog with two
random weapon choices; click one to resume. The weapon pool is Balance (two-hit standard bullets), Laser (four-hit piercing
shots that reflect off one wall) and Bomb (one-hit full-damage area explosions).
Enemies have 4 HP; weapon damage stays fixed across levels. Each level offers two
distinct random weapons. Enemies move at 4.3 blocks/second versus the player's 5,
with persistent waypoints and live actor occupancy for rerouting.
Victory/defeat also opens a clear result dialog with a Play again button.
The temporary gameplay panel shows HP, survival countdown, EXP and equipment.
Restart current configuration resets gameplay; Level 1/2 requests await B's maps.

`core/gameplay/Gameplay` is CPU-only. `ArenaScene::gameplay()` supplies read-only
actor snapshots, stats and per-frame shot/hit events for C's HUD, animation and
instancing work. Interface comments and [A's handoff](docs/member-a-progress.md)
describe coordinate conventions and integration boundaries. No M3 particles or
separate weapon models are included.

## Build with CMake and test with CTest directly

[CMake](https://cmake.org/cmake/help/latest/guide/tutorial/index.html) is the recommended tool for building the project. Other compatible CMake generators and development environments, like Ninja, are also acceptable.

```bash
cmake -S . -B build # If you have experienced with Ninja, you can add `-G Ninja` to the command line to use Ninja generator

cmake --build build
ctest --test-dir build --output-on-failure  # Optional self-tests
```

## Run the application executable directly

After a successful build, the executable is `build/Craft` on macOS/Linux and
`build/Craft.exe` on Windows:

```bash
./build/Craft
```

## Start screen

`TitleScreen` (`src/app/TitleScreen.cpp`) is shown when the application starts, before the arena scene and the Dear ImGui window:

- A pixel-art background image, `assets/textures/title_background.png`, with "Voxel Arena" fading in over it. The text is drawn with `assets/textures/font.png` through `TextRenderer`.
- "PRESS THE ENTER KEY" appears below the title and pulses slowly.
- Pressing **Enter** (or keypad Enter) hides the prompt immediately, holds the screen for 0.3 seconds, then fades to black over 0.6 seconds before the game starts. **ESC** still closes the window at any time.

The current image (272×153, 16:9) is built from the [Mountain at Dusk Background](https://opengameart.org/content/mountain-at-dusk-background) parallax layers.  Text, timings and layout are constants at the top of `TitleScreen.cpp`.  And, `main.cpp` stops drawing the start screen and releases its GPU resources once `TitleScreen::finished()` returns true.

### Changing the Start Screen

To use a different cover image, replace `assets/textures/title_background.png` with another PNG of the same name and rebuild (the build copies `assets/` next to the executable). No shader or code changes are needed:

- Any aspect ratio works, but 16:9 or 4:3 fit best. The image fills the window without stretching; whatever does not fit is cropped evenly from the centre.
- Small pixel-art images are scaled up with nearest-neighbour filtering, so their pixels stay sharp. Large images are scaled down smoothly.
- If the new image is bright where the text sits, adjust `kTitleCentreY` and `kPromptCentreY` in `TitleScreen.cpp` to move the title and prompt.

If you want to change the title and prompt text, edit `kTitle` and `kPrompt` in `TitleScreen.cpp`. To resize the text, change `kDefaultTitleSize` and `kDefaultPromptSize` (the height of one font cell as a fraction of the window height; defaults 0.15 and 0.042), or call `TitleScreen::setTextSizes(titleSize, promptSize)` at runtime. Either line is still shrunk automatically if it would be wider than `kTitleMaxWidth` / `kPromptMaxWidth` of the window.

## 240-frame benchmark

`AppUi` owns the shared Dear ImGui lifecycle and the course-provided benchmark
section. `CustomUi` remains the separate extension point where students may add
assignment-specific controls; it currently displays the arena's movement keys
and dimensions, and exposes no camera controls or camera state.

Keep the code that renders your scene inside the benchmark region, between
`SceneBenchmark::beginSceneFrame()` and `SceneBenchmark::endSceneFrame()`.
This includes code such as clearing the buffers, setting shader uniforms,
and drawing your voxel mesh. Keep ImGui rendering and buffer presentation
outside this measured region.

To clarify more directly, if you keep current code structure and working in `src/main.cpp`:

```cpp
int main() {
  try {
    // ... omitted for brevity
    while (!window.shouldClose()) {
      // ... omitted for brevity
      benchmark.beginSceneFrame();
      // Your voxel scene rendering code goes here
      const bool benchmarkCompleted = benchmark.endSceneFrame();
      if (benchmarkCompleted) {
        printBenchmarkResult(benchmark.snapshot(), framebufferWidth,
          framebufferHeight);
        }
        // ... omitted for brevity
      }
    } catch (const std::exception& error) {
        std::cerr << "Craft failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
```

Use the **Run benchmark** button, or start an automatic run from a terminal:

```bash
CRAFT_BENCHMARK=1 ./build/Craft     
```

The start screen is **skipped** for the environment-triggered run (`CRAFT_BENCHMARK=1`), so the benchmark starts immediately and never waits for Enter. The start screen is also not part of the measured region: it is drawn before the scene, and the **Run benchmark** button only becomes available after you have pressed Enter.

The benchmark disables VSync, performs 60 warm-up frames, measures exactly 240 scene frames with GPU completion synchronisation, prints `average_ms`, then restores
VSync, and excludes Dear ImGui and buffer swap. An environment-triggered run closes automatically after printing its result.

Command-line benchmark results retain `workload=milestone1-voxel-arena` with
simulation frozen before enemies spawn. In-game benchmarks freeze the current
actor snapshot and use `workload=milestone2-gameplay-frozen`. The layout differs from the original starter, so its timing must not be presented as an optimization comparison against the starter workload. It validates the benchmark harness and current scene, but is not final assignment performance evidence. Keep your scene rendering between the `beginSceneFrame` and `endSceneFrame` calls when extending the benchmark.

When improving performance, consider how mesh generation and draw-call count
affect the result. Use the benchmark to compare approaches; no specific
optimisation technique is required.

Please check the [assignment specification](https://gitlab.cse.unsw.edu.au/coursework/COMP3421/26T3/individual-assignment-spec) for more details about submission requirements.

## References

There are some recommended references for C++ and OpenGL learning you may find useful for your assignment.

### C++ Basics

- [Learn C++ Programming for Beginners – Free 31-Hour Course](https://www.freecodecamp.org/news/learn-c-with-free-31-hour-course/)
- [Learn C++ from scratch: The complete guide for beginners](https://www.educative.io/blog/how-to-learn-cpp-the-guide-for-beginners)
- [C++ Tutorial - Cplusplus.com](https://cplusplus.com/doc/tutorial/)
- [C++ Tutorial - W3Schools](https://www.w3schools.com/cpp/)

### C++ Advanced

- **[C++ Reference](https://cppreference.com/)** (The most important and up-to-date documents for C++ programming, you may have learned how to use it in COMP6771)
- [Learn C++](https://www.learncpp.com/)
- [Programming Tutorials - C, C++, OpenGL, STL](https://www.cprogramming.com/tutorial.html)
- Professional C++ (5th Edition), by Marc Gregoire

### OpenGL references

- [LearnOpenGL](https://learnopengl.com/) (Recommended OpenGL learning resource, it provides a lot of examples and explanations for OpenGL)
- [https://www.opengl.org/](https://www.opengl.org/)
- [OpenGL Step by Step](https://ogldev.org/)
- [Khronos OpenGL Wiki](https://wikis.khronos.org/opengl/Main_Page)
- OpenGL SuperBible (5th Edition), by Jr. Richard S. Wright, Nicholas Haemel, Graham Sellers, Benjamin Lipchak
- OpenGL, A Primer , by Edward Angel, Addison Wesley

### Other references
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)
- [GLFW Documentation](https://www.glfw.org/docs/3.3/)
- [GLM Manual](https://github.com/g-truc/glm/blob/master/manual.md)
- [stb library](https://github.com/nothings/stb)
- [NameMC](https://namemc.com/minecraft-skins) (A good website to find Minecraft skins you like, you can download your favourite skin and add it to `assets/textures/` folder for your assignment)
- [Kenney](https://kenney.nl/) (A great source of free game assets)

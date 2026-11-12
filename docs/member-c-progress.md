# Member C progress - models, animation, UI and integration

Updated: 2026-11-11 (Australia/Sydney).

## Completed work

- **Hierarchical player model:** the player is built from a head, torso, two
  arms and two legs, textured with `assets/textures/player.png`. Arms and legs
  swing while moving.
- **Zombie model and animation:** the zombie uses a head, torso, two arms and
  two legs with `assets/textures/zombie.png`. Its arms remain extended while it
  walks; its legs swing, and the attack event raises its arms briefly.
- **Game UI:** the level-up dialog presents up to two upgrade or unlock cards
  horizontally, with simple drawn icons, names, descriptions and combat stats.
  The result dialog offers a full restart. The shared control panel explains WASD movement,
  mouse aiming/shooting and restart controls, and includes a debug panel with
  camera/player positions, geometry counts, frame time, FPS and framebuffer size.
- **Camera and aiming:** the fixed orthographic follow camera was lowered to
  about 60 degrees above the arena plane. Mouse coordinates are projected
  through the rendered view-projection onto the actor-height plane, keeping
  cursor aiming aligned with the changed camera angle and high-DPI window size.
- **Scene integration:** the scene tracks zombie facing, walk phase and attack
  animation by stable enemy ID. Restart resets both the gameplay state and
  player position. The API exposes gameplay snapshots and presentation events
  for effects and batched rendering.
- **Terrain material and lighting first pass:** the floor is split into sand
  and water meshes. Sand uses the repeating dense-noise UV texture at
  `assets/textures/sand_noise_uv.png`, with subtle bump-like variation. Directional
  sunlight, ambient fill, diffuse response and Phong specular highlights shade
  the terrain, obstacles and actors. Water keeps its blue base and uses an
  IOR 1.33 Fresnel term for reflection with a Phong sun highlight. Visual
  acceptance remains pending.
- **Zombie instancing:** the renderer now draws each of the six body parts in
  one instanced batch across all live zombies, preserving per-zombie poses.
  Benchmark comparison data has not yet been recorded.

The previously attempted cursor-following crosshair was reverted. The mouse
cursor itself is used for aiming. Day/Night selection remains disabled until
Member B supplies distinct layouts and loading is connected.

## Files and interfaces

- `src/graphics/ArenaRenderer.cpp` builds and animates the textured player and
  zombie parts.
- `src/app/ArenaScene.cpp` integrates the camera, mouse aim, actor models and
  gameplay event snapshots.
- `src/app/GameplayUi.cpp` draws the gameplay HUD and weapon/result dialogs.
- `src/app/CustomUi.cpp` contains the controls, restart/level placeholders and
  debug panel.
- `src/core/gameplay/Aim.h` converts logical cursor coordinates using the
  framebuffer view-projection matrix.

## Remaining work

Day/Night level switching needs Member B's layouts. The level buttons are
currently disabled. The BRDF and instancing code is in its first pass; record
the baseline-versus-optimized benchmark comparison, refine the materials,
finish visual review and integrate report/video evidence. Weapon recoil and a
drawn crosshair are not yet listed among completed work.

The latest terrain and lighting changes have not yet been compiled or visually
accepted. Manual visual and gameplay acceptance is still pending.

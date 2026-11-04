# Member B progress - Milestone 1

Updated: 2026-11-03 (Australia/Sydney).

## Scope and milestone boundary

ProjectPlan.pdf assigns B world generation and base rendering. This increment
addresses the Week 7 basic voxel map with one deterministic arena. It does not
claim that the team's entire Milestone 1 is complete: enemy spawning/chasing and
shooting/collision gameplay remain Member A's work.

| Milestone | B's deliverable | Status in this increment |
| --- | --- | --- |
| M1 / Week 7 | Basic voxel arena, layout data, integration hooks | Implemented; see validation below |
| M2 / Week 8 | Two Day/Night obstacle layouts, UVs, basic materials and lighting | Pending |
| M3 / Week 9 | Ground/obstacle normal mapping, integration and report evidence | Pending |

## Changes ready to share

- Replaced the starter's four pillars with eight cover groups: four corner
  groups and four side barriers, with an open centre and connected outer lanes.
- Extracted CPU-only `ArenaLayout` data so later layouts do not require changes
  to rendering code. The current layout is not labelled a Day or Night level.
- Added a player spawn and eight enemy entry candidates. `ArenaScene` now uses
  the map's player spawn through the existing `PlayerController::setPosition`.
- Validate obstacle bounds/heights and spawn clearance when constructing a map.
- Added tests for floor/boundary integrity, connected playable space, central
  clearance, spawn reachability, collision compatibility, and malformed layouts.
- Updated the benchmark workload label to `milestone1-voxel-arena`; the timing
  implementation and measurement region are unchanged.

The shared-face removal, combined world mesh, player movement, camera, and
benchmark harness were already supplied by the starter and are not new B work.

## Interfaces for teammates

### Member A

`ArenaWorld::playerSpawn()` and `enemySpawnCandidates()` return actor centres in
world coordinates. The floor occupies Y = 0 through Y = 1; supplied centres are
at Y = 1.45 for starter-sized actors. Initial validation reserves half-extents
`(0.5, 0.45, 0.5)`. A must use each enemy's actual extents with `canOccupy`, adjust
its centre height for its model, and consider live actors and player distance.
Candidates do not implement spawn timing, random selection, AI, or dynamic
collision. `canOccupy` checks solid blocks only.

`setBlock`, `blockAt`, `isSolid`, and `canOccupy` retain their existing contracts.
After edits, call `ArenaScene::rebuildWorldGeometry()` and recheck candidate
positions: spawn metadata is not recomputed after block edits.

### Member C

The renderer and shader vertex format remain unchanged in this increment.
Cover ends at Y = 3, matching the existing camera's framing. Before M2, agree
with B on vertex attribute locations, normal/UV data, material parameters, and
camera/light inputs so instancing and BRDF work can share the rendering contract.
The map uses the existing scene-rendering benchmark region.

The plan mentions metallic weapon materials but also says weapons have no
separate 3D models. Confirm the intended material demonstration objects before
M2; two environment materials are a proposed starting point, not a change to the
agreed plan. B handles basic material parameters; C handles the BRDF challenge.

## Validation

Commands from the repository root:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j 6
ctest --test-dir build --output-on-failure
./build/Craft
CRAFT_BENCHMARK=1 ./build/Craft
```

Verified locally on macOS / Apple Silicon with AppleClang 17:

- CMake configure and full application/test build passed.
- CTest: 5/5 test cases passed (four arena cases plus the existing math case).
- The OpenGL application completed the 60-frame warm-up and 240 measured frames,
  exited successfully, and reported `workload=milestone1-voxel-arena` at 1920x1080.
  This is a runtime smoke check, not an optimization comparison or final report
  benchmark. No baseline/optimized comparison is claimed.
- Interactive visual/walking acceptance remains pending. The desktop inspection
  tool could not attach to this standalone executable, so no screenshot or manual
  movement verification is claimed.

CSE/VLAB validation is still required before submission.

Manual acceptance: press Enter, walk around the centre and through gaps into
the outer lane, check that cover and walls block movement, and confirm all
eight cover groups render. Enemy candidates are metadata and are not drawn.

## GitLab handoff

Suggested commit/MR title: `feat(world): add milestone 1 arena layout and spawn metadata`

Suggested update:

> Member B / M1: implemented a configurable flat arena with eight cover groups,
> a clear player spawn and eight enemy entry candidates. Existing collision and
> rendering hooks are preserved. Added map connectivity and layout validation
> tests. Day/Night layouts, UV/material lighting and normal mapping remain M2/M3
> work. A can now integrate entry candidates; C's vertex/shader interface is
> unchanged.

This workspace has no `.git` metadata. These changes and this progress note are
local; no commit, push, issue, or merge request has been created remotely.

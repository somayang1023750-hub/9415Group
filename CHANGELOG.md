# Changelog

This file records the changes made to the starter code and explain why the issues happened and how we fixed them. Your own ideas and explanations should write in the report, or demonstrate in the short video.



## - 2026-10-05

Initial publish of the starter code for COMP3421/COMP9415 26T3 Computer Graphics group assignment to the staff repos from individual assignment repo.


## - 2026-11-03 — Member B: Milestone 1 map foundation

- Replaced the four starter pillars with eight cover groups, preserving a clear
  central spawn area and connected routes to the outer lane.
- Added CPU-only `ArenaLayout` data and an `ArenaWorld(layout)` constructor to
  separate map configuration from rendering and support later level layouts.
- Added the player spawn and eight enemy spawn candidates; `ArenaScene` now
  initializes the player from the map's spawn metadata. Enemy spawning logic
  remains Member A's responsibility.
- Added validation for obstacle bounds/heights and spawn clearance, plus tests
  for floor/boundary integrity, map connectivity, spawn reachability, collision
  compatibility, and invalid layouts.
- Changed the benchmark workload label to `milestone1-voxel-arena` to distinguish
  the new map from the starter; the benchmark measurement logic is unchanged.
- Updated the README and added [Member B progress and handoff](docs/member-b-progress.md)
  with integration interfaces, milestone boundaries, and verification details.

Validation: macOS build succeeded, all 5 CTest cases passed, and the OpenGL
application completed the 240-frame runtime check. Manual walking/visual checks
and CSE/VLAB verification remain pending.

Scope: this increment covers B's M1 map foundation only. Day/Night layouts,
UVs, materials, and lighting remain M2 work; normal mapping remains M3 work.

## - 2026-11-05 — Member A: Milestone 2 gameplay increment

- Reused existing eight-direction movement; added enemy spawning/chasing and
  swept click-to-shoot bullet collision on B's live arena map.
- Added EXP drops/magnetic collection, paused level-up with randomized weapon
  offers, equipment progression, HP/contact damage, survival outcomes and restart.
- Added CPU-only gameplay snapshots and shot/hit events for B/C integration;
  temporary cube actors and a functional panel leave final visuals/HUD to C.
- Preserved the static command-line benchmark; in-game frozen gameplay uses a
  separate workload label. Timing boundaries remain unchanged.
- Added seven gameplay/movement/aim test cases; all 12 CTest cases passed.
  Linux build, 240-frame OpenGL smoke and automated input/render smoke passed.
- Added [Member A progress and handoff](docs/member-a-progress.md). Day/Night map
  integration, final models/UI, CSE validation and M3 particles remain pending.


## - 2026-11-05 — Upgrade selection usability and weapon tuning

- Moved upgrade choices into a centred English modal with an explicit paused
  message, full-width buttons and post-upgrade weapon stats. Selection resumes
  gameplay; old/collapsed HUD sizes cannot hide the choices.
- Centralized weapon profiles: Heavy has slower shots/projectiles and higher
  damage; Rapid has faster shots/projectiles and lower damage; Balanced sits
  between them. Set enemy HP to 3 to distinguish required hits.
- Build and all 13 CTest cases passed; separate ImGui interaction checks verified
  the modal and resume flow at 640x480, 1280x720 and 1920x1080.

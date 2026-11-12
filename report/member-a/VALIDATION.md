# Capture workspace verification

- Based on gameplay revision 2a5ea68; Word descriptions reference this gameplay
  baseline. This increment adds a report capture target and extracts a shared
  original trail helper; it does not redesign game particles or models.
- Full Linux C++17 Release builds: Craft, MemberAShowcase and craft_tests passed.
- All 48 CTest cases passed after the shared-helper extraction.
- Xvfb/Mesa `MemberAShowcase --smoke` passed: 128 frames covering the four pages,
  Day/Night lighting, actual upgrade/victory/defeat phases, PNG output and GL error
  checks. Captures were visually inspected for models, impacts, fire and UI.
- Fixture phase setup asserts the actual Gameplay phase. UI is not an imitation.
- The trail helper body matches the previous ArenaScene block after changing only
  the receiver name and the equivalent Bomb condition. Ground fire and impact
  emitters are called directly from production ParticleEffects.
- Word preserves original teammate paragraphs, tables and images except the
  particle section explicitly authorized for rewriting. A1–A5 image/code slots
  remain for the user's final captures; the original teammate Figure 1/2 labels
  are retained without renumbering them.
- No current Windows execution or comparative hardware-performance result is
  claimed. Build on Windows using build_windows.cmd or the documented CMake flag.

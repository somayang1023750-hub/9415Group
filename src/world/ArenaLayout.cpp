#include "world/ArenaLayout.h"

namespace craft::world {

ArenaLayout makeMilestoneOneLayout() {
  ArenaLayout layout;
  // Four corner cover groups and four longer side barriers. Gaps between
  // groups connect the central fighting space to a continuous outer lane.
  // Everything stays at or below Y = 3, matching the existing camera framing.
  layout.obstacles = {
      {6, 6, 3, 2, 2},   {23, 6, 3, 2, 2},
      {6, 24, 3, 2, 2},  {23, 24, 3, 2, 2},
      {5, 13, 2, 6, 2},  {25, 13, 2, 6, 2},
      {13, 5, 6, 2, 2},  {13, 25, 6, 2, 2},
  };
  // Candidate entry positions only: A owns spawn timing, occupancy checks,
  // enemy size, and any minimum distance from the moving player.
  layout.enemySpawnCandidates = {
      {16.0F, 1.45F, 2.5F},  {29.5F, 1.45F, 16.0F},
      {16.0F, 1.45F, 29.5F}, {2.5F, 1.45F, 16.0F},
      {3.5F, 1.45F, 3.5F},   {28.5F, 1.45F, 3.5F},
      {3.5F, 1.45F, 28.5F},  {28.5F, 1.45F, 28.5F},
  };
  return layout;
}

}  // namespace craft::world

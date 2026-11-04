#pragma once

#include <vector>

#include <glm/vec3.hpp>

namespace craft::world {

// Axis-aligned cover placed on top of the floor (Y = 1).
struct ArenaObstacle {
  int x;
  int z;
  int width;
  int depth;
  int height;
};

// CPU-only level data. Day/Night layouts can use this same interface in M2.
// Spawn positions are actor centres in world coordinates, not block indices.
struct ArenaLayout {
  std::vector<ArenaObstacle> obstacles;
  glm::vec3 playerSpawn{16.0F, 1.45F, 16.0F};
  std::vector<glm::vec3> enemySpawnCandidates;
};

// One deterministic, flat arena for Milestone 1; not a Day/Night level pair.
[[nodiscard]] ArenaLayout makeMilestoneOneLayout();

}  // namespace craft::world

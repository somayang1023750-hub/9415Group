#pragma once

#include <cstdint>
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

// One floor cell that belongs to a visually marked water region.
struct ArenaWaterCell {
  int x;
  int z;
};

enum class ArenaLevel { Day, Night };

// CPU-only level data shared by mesh generation and collision.
// Spawn positions are actor centres in world coordinates, not block indices.
struct ArenaLayout {
  std::vector<ArenaObstacle> obstacles;
  std::vector<ArenaWaterCell> waterCells;
  glm::vec3 playerSpawn{16.0F, 1.45F, 16.0F};
  std::vector<glm::vec3> enemySpawnCandidates;
};

[[nodiscard]] ArenaLayout makeArenaLayout(ArenaLevel level);

// A flat arena whose cover positions are sampled from a seeded noise map.
// The no-argument overload uses the assignment's reproducible seed (9415);
// the seeded overload is useful when a caller needs a different fixed arena.
[[nodiscard]] ArenaLayout makeMilestoneOneLayout();
[[nodiscard]] ArenaLayout makeMilestoneOneLayout(std::uint32_t seed);

}  // namespace craft::world

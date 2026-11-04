#include <array>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "world/ArenaLayout.h"
#include "world/ArenaWorld.h"

namespace {

using craft::world::ArenaWorld;
using craft::world::BlockType;
const glm::vec3 kPlayerHalfExtents{0.42F, 0.44F, 0.42F};

}  // namespace

TEST_CASE("Arena retains a solid floor and closed low boundary", "[world]") {
  const ArenaWorld world;
  for (int z = 0; z < ArenaWorld::kDepth; ++z) {
    for (int x = 0; x < ArenaWorld::kWidth; ++x) {
      REQUIRE(world.blockAt(x, 0, z) == BlockType::Floor);
      if (x == 0 || x == ArenaWorld::kWidth - 1 ||
          z == 0 || z == ArenaWorld::kDepth - 1) {
        REQUIRE(world.blockAt(x, 1, z) == BlockType::Boundary);
        REQUIRE(world.blockAt(x, 2, z) == BlockType::Boundary);
      }
      // Existing camera framing assumes all cover ends at Y = 3.
      for (int y = 3; y < ArenaWorld::kHeight; ++y) {
        REQUIRE(world.blockAt(x, y, z) == BlockType::Air);
      }
    }
  }
}

TEST_CASE("All walkable cells and entry candidates connect to the player spawn",
          "[world][layout]") {
  const ArenaWorld world;
  constexpr int width = ArenaWorld::kWidth;
  std::array<bool, width * ArenaWorld::kDepth> visited{};
  std::queue<std::pair<int, int>> pending;
  const int spawnX = static_cast<int>(std::floor(world.playerSpawn().x));
  const int spawnZ = static_cast<int>(std::floor(world.playerSpawn().z));
  pending.emplace(spawnX, spawnZ);
  visited[spawnZ * width + spawnX] = true;

  const auto walkable = [&](int x, int z) {
    return world.canOccupy({x + 0.5F, 1.45F, z + 0.5F}, kPlayerHalfExtents);
  };
  REQUIRE(walkable(spawnX, spawnZ));
  constexpr int steps[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  while (!pending.empty()) {
    const auto [x, z] = pending.front();
    pending.pop();
    for (const auto& step : steps) {
      const int nextX = x + step[0];
      const int nextZ = z + step[1];
      if (nextX < 0 || nextX >= width || nextZ < 0 ||
          nextZ >= ArenaWorld::kDepth) {
        continue;
      }
      const int index = nextZ * width + nextX;
      if (!visited[index] && walkable(nextX, nextZ)) {
        visited[index] = true;
        pending.emplace(nextX, nextZ);
      }
    }
  }
  for (int z = 1; z < ArenaWorld::kDepth - 1; ++z) {
    for (int x = 1; x < width - 1; ++x) {
      CAPTURE(x, z);
      if (walkable(x, z)) {
        REQUIRE(visited[z * width + x]);
      }
    }
  }

  // A safe initial movement area, not just a single unoccupied spawn point.
  for (int dz = -2; dz <= 2; ++dz) {
    for (int dx = -2; dx <= 2; ++dx) {
      REQUIRE(world.canOccupy(world.playerSpawn() + glm::vec3(dx, 0, dz),
                             kPlayerHalfExtents));
    }
  }
  REQUIRE_FALSE(world.enemySpawnCandidates().empty());
  for (const auto& spawn : world.enemySpawnCandidates()) {
    REQUIRE(world.canOccupy(spawn, kPlayerHalfExtents));
    const int x = static_cast<int>(std::floor(spawn.x));
    const int z = static_cast<int>(std::floor(spawn.z));
    REQUIRE(visited[z * width + x]);
  }
}

TEST_CASE("Layout data drives cover, spawn metadata and collision", "[world]") {
  craft::world::ArenaLayout layout;
  layout.obstacles = {{10, 10, 3, 2, 2}};
  layout.playerSpawn = {4.5F, 1.45F, 4.5F};
  layout.enemySpawnCandidates = {{27.5F, 1.45F, 27.5F}};
  ArenaWorld world(layout);

  REQUIRE(world.playerSpawn() == layout.playerSpawn);
  REQUIRE(world.enemySpawnCandidates() == layout.enemySpawnCandidates);
  REQUIRE_FALSE(world.canOccupy({11.5F, 1.45F, 10.5F}, kPlayerHalfExtents));
  REQUIRE(world.canOccupy({9.5F, 1.45F, 10.5F}, kPlayerHalfExtents));
  REQUIRE_FALSE(world.canOccupy({0.5F, 1.45F, 16.0F}, kPlayerHalfExtents));
  REQUIRE_FALSE(world.canOccupy({-1.0F, 1.45F, 16.0F}, kPlayerHalfExtents));
  REQUIRE_FALSE(world.canOccupy({16.0F, 0.5F, 16.0F}, kPlayerHalfExtents));

  // Preserve the existing map-editing contract used by gameplay.
  world.setBlock(11, 1, 10, BlockType::Air);
  REQUIRE(world.canOccupy({11.5F, 1.45F, 10.5F}, kPlayerHalfExtents));
  REQUIRE_THROWS_AS(world.setBlock(32, 1, 10, BlockType::Obstacle),
                    std::out_of_range);
}

TEST_CASE("Malformed layouts fail before they can become playable", "[world]") {
  auto layout = craft::world::makeMilestoneOneLayout();
  SECTION("Cover cannot replace the boundary") {
    layout.obstacles.push_back({0, 5, 2, 2, 2});
  }
  SECTION("Oversized cover does not overflow coordinate arithmetic") {
    layout.obstacles.push_back({5, 5, std::numeric_limits<int>::max(), 2, 2});
  }
  SECTION("Cover cannot exceed the camera framing") {
    layout.obstacles.push_back({10, 10, 2, 2, 3});
  }
  SECTION("Player spawn cannot intersect cover") {
    layout.obstacles.push_back({15, 15, 2, 2, 2});
  }
  SECTION("Enemy entry cannot intersect the boundary") {
    layout.enemySpawnCandidates.push_back({0.5F, 1.45F, 16.0F});
  }
  SECTION("Spawn must rest on the floor") {
    layout.playerSpawn.y = 4.0F;
  }
  SECTION("Spawn cannot have non-finite coordinates") {
    layout.playerSpawn.x = std::numeric_limits<float>::quiet_NaN();
  }
  REQUIRE_THROWS_AS(static_cast<void>(ArenaWorld{layout}), std::invalid_argument);
}

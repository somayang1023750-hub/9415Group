#include "world/ArenaWorld.h"

#include <cmath>
#include <stdexcept>

namespace craft::world {

ArenaWorld::ArenaWorld() : ArenaWorld(makeArenaLayout(ArenaLevel::Day)) {}

ArenaWorld::ArenaWorld(const ArenaLayout& layout)
    : playerSpawn_(layout.playerSpawn),
      enemySpawnCandidates_(layout.enemySpawnCandidates) {
  for (int z = 0; z < kDepth; ++z) {
    for (int x = 0; x < kWidth; ++x) {
      cells_[index(x, 0, z)] = BlockType::Floor;
    }
  }

  for (const auto& waterCell : layout.waterCells) {
    if (waterCell.x < 1 || waterCell.x >= kWidth - 1 || waterCell.z < 1 ||
        waterCell.z >= kDepth - 1) {
      throw std::invalid_argument(
          "Arena water cells must stay inside the boundary");
    }
    const std::size_t column =
        static_cast<std::size_t>(waterCell.z) * kWidth + waterCell.x;
    if (waterCells_[column]) continue;
    if (waterGroundArea_ >= kMaximumWaterArea) {
      throw std::invalid_argument(
          "Arena water cannot cover more than one quarter of the floor");
    }
    waterCells_[column] = true;
    ++waterGroundArea_;
  }

  for (int y = 1; y <= 2; ++y) {
    for (int i = 0; i < kWidth; ++i) {
      cells_[index(0, y, i)] = BlockType::Boundary;
      cells_[index(kWidth - 1, y, i)] = BlockType::Boundary;
      cells_[index(i, y, 0)] = BlockType::Boundary;
      cells_[index(i, y, kDepth - 1)] = BlockType::Boundary;
    }
  }

  std::array<bool, static_cast<std::size_t>(kWidth) * kDepth> obstacleColumns{};
  for (const auto& obstacle : layout.obstacles) {
    // Keep the boundary intact and cover inside the current camera's height.
    // Subtraction avoids overflow when validating externally supplied sizes.
    if (obstacle.x < 1 || obstacle.x >= kWidth - 1 ||
        obstacle.z < 1 || obstacle.z >= kDepth - 1 ||
        obstacle.width < 1 || obstacle.width > kWidth - 1 - obstacle.x ||
        obstacle.depth < 1 || obstacle.depth > kDepth - 1 - obstacle.z ||
        obstacle.height < 1 || obstacle.height > 2) {
      throw std::invalid_argument("Arena obstacle must fit inside the boundary "
                                  "and be one or two blocks tall");
    }
    for (int dz = 0; dz < obstacle.depth; ++dz) {
      for (int dx = 0; dx < obstacle.width; ++dx) {
        const int x = obstacle.x + dx;
        const int z = obstacle.z + dz;
        const std::size_t column = static_cast<std::size_t>(z) * kWidth + x;
        if (waterCells_[column]) {
          throw std::invalid_argument(
              "Arena obstacles cannot cover water region cells");
        }
        if (!obstacleColumns[column]) {
          if (obstacleGroundArea_ >= kMaximumObstacleArea) {
            throw std::invalid_argument(
                "Arena obstacles cannot cover more than one fifth of the floor");
          }
          obstacleColumns[column] = true;
          ++obstacleGroundArea_;
        }
        for (int y = 1; y <= obstacle.height; ++y) {
          cells_[index(x, y, z)] = BlockType::Obstacle;
        }
      }
    }
  }

  // Reserve a full block of horizontal clearance for starter-sized actors.
  // Larger enemies must check their own extents when A selects a candidate.
  const glm::vec3 spawnHalfExtents{0.5F, 0.45F, 0.5F};
  const auto validSpawn = [&](const glm::vec3& position) {
    return std::abs(position.y - 1.45F) < 0.0001F &&
           canOccupy(position, spawnHalfExtents);
  };
  if (!validSpawn(playerSpawn_)) {
    throw std::invalid_argument("Player spawn must be clear and on the arena floor");
  }
  for (const auto& position : enemySpawnCandidates_) {
    if (!validSpawn(position)) {
      throw std::invalid_argument("Enemy spawn candidate must be clear and on the arena floor");
    }
  }
}

const glm::vec3& ArenaWorld::playerSpawn() const noexcept {
  return playerSpawn_;
}

const std::vector<glm::vec3>& ArenaWorld::enemySpawnCandidates() const noexcept {
  return enemySpawnCandidates_;
}

BlockType ArenaWorld::blockAt(int x, int y, int z) const noexcept {
  if (!inBounds(x, y, z)) {
    return BlockType::Air;
  }
  return cells_[index(x, y, z)];
}

bool ArenaWorld::isSolid(int x, int y, int z) const noexcept {
  if (!inBounds(x, y, z)) {
    return true;
  }
  return blockAt(x, y, z) != BlockType::Air;
}

bool ArenaWorld::isWaterSurface(int x, int z) const noexcept {
  if (x < 0 || x >= kWidth || z < 0 || z >= kDepth) return false;
  return waterCells_[static_cast<std::size_t>(z) * kWidth + x];
}

void ArenaWorld::setBlock(int x, int y, int z, BlockType type) {
  if (!inBounds(x, y, z)) {
    throw std::out_of_range("ArenaWorld cell is outside the 32 x 32 x 32 grid");
  }

  bool hasObstacleInColumn = false;
  bool hasOtherObstacleInColumn = false;
  for (int columnY = 0; columnY < kHeight; ++columnY) {
    if (cells_[index(x, columnY, z)] != BlockType::Obstacle) continue;
    hasObstacleInColumn = true;
    if (columnY != y) hasOtherObstacleInColumn = true;
  }

  if (type == BlockType::Obstacle && !hasObstacleInColumn) {
    if (obstacleGroundArea_ >= kMaximumObstacleArea) {
      throw std::invalid_argument(
          "Arena obstacles cannot cover more than one fifth of the floor");
    }
    ++obstacleGroundArea_;
  } else if (type != BlockType::Obstacle && hasObstacleInColumn &&
             !hasOtherObstacleInColumn) {
    --obstacleGroundArea_;
  }
  cells_[index(x, y, z)] = type;
}

bool ArenaWorld::canOccupy(const glm::vec3& center,
                           const glm::vec3& halfExtents) const noexcept {
  constexpr float kEdgeEpsilon = 0.0001F;
  if (!std::isfinite(center.x) || !std::isfinite(center.y) ||
      !std::isfinite(center.z) || !std::isfinite(halfExtents.x) ||
      !std::isfinite(halfExtents.y) || !std::isfinite(halfExtents.z) ||
      halfExtents.x < 0.0F || halfExtents.y < 0.0F ||
      halfExtents.z < 0.0F) {
    return false;
  }

  const glm::vec3 minimum = center - halfExtents;
  const glm::vec3 maximum = center + halfExtents;
  if (minimum.x < 0.0F || minimum.y < 0.0F || minimum.z < 0.0F ||
      maximum.x > kWidth || maximum.y > kHeight || maximum.z > kDepth) {
    return false;
  }

  const int minX =
      static_cast<int>(std::floor(minimum.x + kEdgeEpsilon));
  const int maxX =
      static_cast<int>(std::floor(maximum.x - kEdgeEpsilon));
  const int minY =
      static_cast<int>(std::floor(minimum.y + kEdgeEpsilon));
  const int maxY =
      static_cast<int>(std::floor(maximum.y - kEdgeEpsilon));
  const int minZ =
      static_cast<int>(std::floor(minimum.z + kEdgeEpsilon));
  const int maxZ =
      static_cast<int>(std::floor(maximum.z - kEdgeEpsilon));

  for (int y = minY; y <= maxY; ++y) {
    for (int z = minZ; z <= maxZ; ++z) {
      for (int x = minX; x <= maxX; ++x) {
        if (isSolid(x, y, z)) {
          return false;
        }
      }
    }
  }
  return true;
}

bool ArenaWorld::inBounds(int x, int y, int z) noexcept {
  return x >= 0 && x < kWidth && y >= 0 && y < kHeight && z >= 0 &&
         z < kDepth;
}

std::size_t ArenaWorld::index(int x, int y, int z) noexcept {
  return (static_cast<std::size_t>(y) * kDepth + z) * kWidth + x;
}

}  // namespace craft::world

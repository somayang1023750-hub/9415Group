#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <glm/vec3.hpp>

#include "world/ArenaLayout.h"

namespace craft::world {

enum class BlockType : std::uint8_t {
  Air,
  Floor,
  Boundary,
  Obstacle,
};

// Editable 32 x 32 x 32 block volume. Coordinates use X/Z for the arena
// plane and Y for height; cell (x, 0, z) is the top layer of the floor.
class ArenaWorld {
 public:
  static constexpr int kWidth = 32;
  static constexpr int kHeight = 32;
  static constexpr int kDepth = 32;
  static constexpr float kBlockSize = 1.0F;
  static constexpr std::size_t kCellCount =
      static_cast<std::size_t>(kWidth) * kHeight * kDepth;

  ArenaWorld();
  explicit ArenaWorld(const ArenaLayout& layout);

  [[nodiscard]] const glm::vec3& playerSpawn() const noexcept;
  // Layout hints, not a live occupancy guarantee after setBlock/gameplay edits.
  [[nodiscard]] const std::vector<glm::vec3>& enemySpawnCandidates() const noexcept;

  [[nodiscard]] BlockType blockAt(int x, int y, int z) const noexcept;
  [[nodiscard]] bool isSolid(int x, int y, int z) const noexcept;

  // Map-generation hook for teammates. Out-of-range coordinates are rejected.
  void setBlock(int x, int y, int z, BlockType type);

  [[nodiscard]] bool canOccupy(const glm::vec3& center,
                               const glm::vec3& halfExtents) const noexcept;

 private:
  [[nodiscard]] static bool inBounds(int x, int y, int z) noexcept;
  [[nodiscard]] static std::size_t index(int x, int y, int z) noexcept;

  std::array<BlockType, kCellCount> cells_{};
  glm::vec3 playerSpawn_;
  std::vector<glm::vec3> enemySpawnCandidates_;
};

}  // namespace craft::world

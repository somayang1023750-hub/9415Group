#pragma once

#include <array>
#include <filesystem>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <optional>
#include <vector>

#include "core/gameplay/EnemyTypes.h"
#include "graphics/Texture.h"

namespace craft::world {
class ArenaWorld;
enum class ArenaLevel;
}  // namespace craft::world

namespace craft::graphics {

class Shader;

// GPU-side arena mesh builder. It combines exposed block faces into one draw
// call, draws the player's six parts, and batches zombie parts by instance.
class ArenaRenderer {
 public:
  struct ZombieInstance {
    glm::vec3 position{0.0F};
    float yawRadians = 0.0F;
    float walkCyclePhase = 0.0F;
    bool moving = false;
    float attackAnimationTime = -1.0F;
    gameplay::EnemyKind kind = gameplay::EnemyKind::Zombie;
  };

  static constexpr int kPlayerPartCount = 6;
  static constexpr int kZombiePartCount = 6;
  static constexpr int kSpiderPartCount = 10;
  static int enemyPartCount(gameplay::EnemyKind kind) {
    return kind == gameplay::EnemyKind::Spider ? kSpiderPartCount
                                               : kZombiePartCount;
  }
  static constexpr int kTrianglesPerPlayerPart = 12;
  static constexpr float kZombieAttackAnimationSeconds = 0.54F;

  ArenaRenderer(const world::ArenaWorld& world,
                const std::filesystem::path& playerTexturePath,
                const std::filesystem::path& zombieTexturePath,
                const std::filesystem::path& sandTexturePath);
  ~ArenaRenderer();

  ArenaRenderer(const ArenaRenderer&) = delete;
  ArenaRenderer& operator=(const ArenaRenderer&) = delete;

  // Call after changing the world through ArenaWorld::setBlock.
  void rebuildWorldMesh(const world::ArenaWorld& world);

  void drawWorld(const Shader& shader, const glm::mat4& viewProjection,
                 const glm::vec3& eyePosition, world::ArenaLevel level,
                 const glm::vec3& playerPosition, bool normalMapping) const;
  void drawPlayer(const Shader& shader, const glm::mat4& viewProjection,
                  const glm::vec3& position, float yawRadians,
                  float walkCyclePhase, bool moving) const;
  // C: legacy API name, now batches all three kinds (six parts per biped,
  // ten per spider). Batch count depends on present kinds, not actor count.
  // If the optional skin asset is missing, use a green flat-shaded model.
  void drawZombies(const Shader& shader, const glm::mat4& viewProjection,
                   const std::vector<ZombieInstance>& zombies) const;

  // C: temporary untextured cube proxies; replace with your actor/instanced
  // renderer using Gameplay snapshots without changing simulation ownership.
  void drawProxy(const Shader& shader, const glm::mat4& viewProjection,
                 const glm::vec3& position, const glm::vec3& halfExtents,
                 const glm::vec3& color, float yawRadians = 0,
                 bool metallic = false) const;
  void drawBlast(const Shader& shader, const glm::mat4& viewProjection,
                 const glm::vec3& position, float radius, float age) const;

  [[nodiscard]] int worldVertexCount() const noexcept;
  [[nodiscard]] int worldDrawCallCount() const noexcept;

 private:
  struct PlayerPartRange {
    int firstVertex = 0;
    int vertexCount = 0;
  };

  unsigned int blastVertexArray_ = 0;
  unsigned int blastVertexBuffer_ = 0;
  Texture playerTexture_;
  Texture sandTexture_;
  Texture sandNormal_;
  Texture masonry_;
  Texture masonryNormal_;
  std::optional<Texture> zombieTexture_;
  std::optional<Texture> golemTexture_;
  std::optional<Texture> spiderTexture_;
  unsigned int worldVertexArray_ = 0;
  unsigned int worldVertexBuffer_ = 0;
  int worldVertexCount_ = 0;
  int worldDrawCallCount_ = 0;
  unsigned int sandVertexArray_ = 0;
  unsigned int sandVertexBuffer_ = 0;
  int sandVertexCount_ = 0;
  unsigned int waterVertexArray_ = 0;
  unsigned int waterVertexBuffer_ = 0;
  int waterVertexCount_ = 0;

  unsigned int playerVertexArray_ = 0;
  unsigned int playerVertexBuffer_ = 0;
  std::array<PlayerPartRange, kPlayerPartCount> playerPartRanges_{};
  unsigned int zombieVertexArray_ = 0;
  unsigned int zombieVertexBuffer_ = 0;
  unsigned int zombieInstanceBuffer_ = 0;
  std::array<PlayerPartRange, kZombiePartCount> zombiePartRanges_{};
};

}  // namespace craft::graphics

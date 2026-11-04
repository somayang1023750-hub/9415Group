#pragma once

#include <array>
#include <filesystem>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "graphics/Texture.h"

namespace craft::world {
class ArenaWorld;
}

namespace craft::graphics {

class Shader;

// GPU-side arena mesh builder. It combines exposed block faces into one draw
// call and draws the textured player as six independently transformable parts.
class ArenaRenderer {
 public:
  static constexpr int kPlayerPartCount = 6;
  static constexpr int kTrianglesPerPlayerPart = 12;

  ArenaRenderer(const world::ArenaWorld& world,
                const std::filesystem::path& playerTexturePath);
  ~ArenaRenderer();

  ArenaRenderer(const ArenaRenderer&) = delete;
  ArenaRenderer& operator=(const ArenaRenderer&) = delete;

  // Call after changing the world through ArenaWorld::setBlock.
  void rebuildWorldMesh(const world::ArenaWorld& world);

  void drawWorld(const Shader& shader, const glm::mat4& viewProjection) const;
  void drawPlayer(const Shader& shader, const glm::mat4& viewProjection,
                  const glm::vec3& position, float yawRadians,
                  float walkCyclePhase, bool moving) const;

  // C: temporary untextured cube proxies; replace with your actor/instanced
  // renderer using Gameplay snapshots without changing simulation ownership.
  void drawProxy(const Shader& shader, const glm::mat4& viewProjection,
                 const glm::vec3& position, const glm::vec3& halfExtents,
                 const glm::vec3& color) const;

  [[nodiscard]] int worldVertexCount() const noexcept;

 private:
  struct PlayerPartRange {
    int firstVertex = 0;
    int vertexCount = 0;
  };

  Texture playerTexture_;
  unsigned int worldVertexArray_ = 0;
  unsigned int worldVertexBuffer_ = 0;
  int worldVertexCount_ = 0;

  unsigned int playerVertexArray_ = 0;
  unsigned int playerVertexBuffer_ = 0;
  std::array<PlayerPartRange, kPlayerPartCount> playerPartRanges_{};
};

}  // namespace craft::graphics

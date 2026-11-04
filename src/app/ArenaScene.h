#pragma once

#include <filesystem>

#include "app/FollowCamera.h"
#include "app/PlayerController.h"
#include "core/gameplay/Gameplay.h"
#include "graphics/ArenaRenderer.h"
#include "world/ArenaWorld.h"

namespace craft::graphics {
class Shader;
}

namespace craft::app {

// Small integration point for the starter gameplay loop. Other systems can
// query/edit the world and player separately, then ask this scene to rebuild
// its mesh after changing blocks.
class ArenaScene {
 public:
  explicit ArenaScene(const std::filesystem::path& playerTexturePath);

  void update(const MovementInput& input, float deltaSeconds,
              std::optional<glm::vec3> shotTarget = std::nullopt);
  void resetGameplay();
  [[nodiscard]] gameplay::Gameplay& gameplay() noexcept { return gameplay_; }
  [[nodiscard]] const gameplay::Gameplay& gameplay() const noexcept {
    return gameplay_;
  }
  void render(const graphics::Shader& shader, int framebufferWidth,
              int framebufferHeight) const;
  void rebuildWorldGeometry();

  [[nodiscard]] world::ArenaWorld& world() noexcept;
  [[nodiscard]] const world::ArenaWorld& world() const noexcept;
  [[nodiscard]] PlayerController& player() noexcept;
  [[nodiscard]] const PlayerController& player() const noexcept;
  [[nodiscard]] const FollowCamera& camera() const noexcept;
  [[nodiscard]] int worldVertexCount() const noexcept;

 private:
  gameplay::Gameplay gameplay_;
  world::ArenaWorld world_;
  PlayerController player_;
  FollowCamera camera_;
  graphics::ArenaRenderer renderer_;
};

}  // namespace craft::app

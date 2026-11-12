#pragma once

#include <filesystem>
#include <optional>
#include <unordered_map>

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
  struct MouseAimInput {
    double x = 0.0;
    double y = 0.0;
    int windowWidth = 0;
    int windowHeight = 0;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
  };

  ArenaScene(const std::filesystem::path& playerTexturePath,
             const std::filesystem::path& zombieTexturePath,
             const std::filesystem::path& sandTexturePath);

  void update(const MovementInput& input, float deltaSeconds,
              std::optional<glm::vec3> shotTarget = std::nullopt);
  // Projects a mouse click after movement, using the camera transform that
  // will render this frame. Direct world targets remain available above.
  void updateFromMouse(const MovementInput& input, float deltaSeconds,
                       const MouseAimInput& aimInput);
  void resetGameplay();
  void selectLevel(world::ArenaLevel level);
  [[nodiscard]] world::ArenaLevel level() const noexcept { return level_; }
  [[nodiscard]] gameplay::Gameplay& gameplay() noexcept { return gameplay_; }
  [[nodiscard]] const gameplay::Gameplay& gameplay() const noexcept {
    return gameplay_;
  }
  void render(const graphics::Shader& shader, int framebufferWidth,
              int framebufferHeight, bool normalMapping = true) const;
  void rebuildWorldGeometry();

  [[nodiscard]] world::ArenaWorld& world() noexcept;
  [[nodiscard]] const world::ArenaWorld& world() const noexcept;
  [[nodiscard]] PlayerController& player() noexcept;
  [[nodiscard]] const PlayerController& player() const noexcept;
  [[nodiscard]] const FollowCamera& camera() const noexcept;
  [[nodiscard]] int worldVertexCount() const noexcept;
  [[nodiscard]] int worldDrawCallCount() const noexcept;

 private:
  struct EnemyRenderState {
    glm::vec3 previousPosition{0.0F};
    float walkCyclePhase = 0.0F;
    float yawRadians = 0.0F;
    float attackAnimationTime = -1.0F;
    bool moving = false;
  };

  void updateInternal(const MovementInput& input, float deltaSeconds,
                      std::optional<glm::vec3> shotTarget,
                      const MouseAimInput* aimInput);

  gameplay::Gameplay gameplay_;
  world::ArenaLevel level_ = world::ArenaLevel::Day;
  world::ArenaWorld world_;
  PlayerController player_;
  FollowCamera camera_;
  graphics::ArenaRenderer renderer_;
  std::unordered_map<std::uint32_t, EnemyRenderState> enemyRenderStates_;
};

}  // namespace craft::app

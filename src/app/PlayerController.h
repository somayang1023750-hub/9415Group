#pragma once

#include <glm/vec3.hpp>

namespace craft::world {
class ArenaWorld;
}

namespace craft::app {

struct MovementInput {
  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
};

// Gameplay-facing movement interface. Input is camera-relative so the same
// WASD mapping remains intuitive with the fixed diagonal arena camera.
class PlayerController {
 public:
  PlayerController();

  void update(const MovementInput& input, float deltaSeconds,
              const glm::vec3& cameraForward,
              const glm::vec3& cameraRight,
              const world::ArenaWorld& world);

  [[nodiscard]] const glm::vec3& position() const noexcept;
  [[nodiscard]] const glm::vec3& facingDirection() const noexcept;
  [[nodiscard]] float yawRadians() const noexcept;
  [[nodiscard]] bool isMoving() const noexcept;
  [[nodiscard]] float walkCyclePhase() const noexcept;
  [[nodiscard]] float moveSpeed() const noexcept;

  // Useful for spawning, resets, and later multiplayer/game-state code.
  [[nodiscard]] bool setPosition(const glm::vec3& position,
                                 const world::ArenaWorld& world);
  void setMoveSpeed(float blocksPerSecond);

 private:
  [[nodiscard]] static const glm::vec3& collisionHalfExtents() noexcept;

  glm::vec3 position_{16.0F, 1.45F, 16.0F};
  glm::vec3 facingDirection_{0.0F, 0.0F, 1.0F};
  float yawRadians_ = 0.0F;
  float moveSpeed_ = 5.0F;
  float walkCyclePhase_ = 0.0F;
  bool moving_ = false;
};

}  // namespace craft::app

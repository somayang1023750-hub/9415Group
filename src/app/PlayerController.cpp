#include "app/PlayerController.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <stdexcept>

#include "core/gameplay/ActorGeometry.h"
#include "world/ArenaWorld.h"

namespace craft::app {

namespace {

constexpr float kMaximumFrameStep = 0.1F;
constexpr float kDirectionEpsilon = 0.0001F;
constexpr float kWalkCycleRadiansPerSecond = 10.0F;
constexpr float kTwoPi = 6.28318530718F;
const glm::vec3 kPlayerCollisionHalfExtents = gameplay::playerHalfExtents();

glm::vec3 horizontalUnit(const glm::vec3& vector) {
  const glm::vec3 horizontal{vector.x, 0.0F, vector.z};
  const float magnitude = glm::length(horizontal);
  if (magnitude <= kDirectionEpsilon) {
    return glm::vec3{0.0F};
  }
  return horizontal / magnitude;
}

}  // namespace

PlayerController::PlayerController() = default;

void PlayerController::update(const MovementInput& input, float deltaSeconds,
                              const glm::vec3& cameraForward,
                              const glm::vec3& cameraRight,
                              const world::ArenaWorld& world) {
  glm::vec3 direction{0.0F};
  const glm::vec3 forward = horizontalUnit(cameraForward);
  const glm::vec3 right = horizontalUnit(cameraRight);

  if (input.forward) {
    direction += forward;
  }
  if (input.backward) {
    direction -= forward;
  }
  if (input.right) {
    direction += right;
  }
  if (input.left) {
    direction -= right;
  }

  const float directionLength = glm::length(direction);
  if (directionLength <= kDirectionEpsilon) {
    moving_ = false;
    walkCyclePhase_ = 0.0F;
    return;
  }

  direction /= directionLength;
  facingDirection_ = direction;
  yawRadians_ = std::atan2(direction.x, direction.z);

  const float safeDelta = std::clamp(deltaSeconds, 0.0F, kMaximumFrameStep);
  const glm::vec3 displacement = direction * (moveSpeed_ * safeDelta);
  moving_ = false;

  // Resolve each horizontal axis independently so the player can slide along
  // arena walls and obstacle faces instead of stopping at a corner.
  glm::vec3 candidate = position_;
  candidate.x += displacement.x;
  if (world.canOccupy(candidate, collisionHalfExtents())) {
    position_.x = candidate.x;
    moving_ = moving_ || std::abs(displacement.x) > kDirectionEpsilon;
  }

  candidate = position_;
  candidate.z += displacement.z;
  if (world.canOccupy(candidate, collisionHalfExtents())) {
    position_.z = candidate.z;
    moving_ = moving_ || std::abs(displacement.z) > kDirectionEpsilon;
  }

  if (moving_) {
    walkCyclePhase_ = std::fmod(
        walkCyclePhase_ + safeDelta * kWalkCycleRadiansPerSecond, kTwoPi);
  } else {
    walkCyclePhase_ = 0.0F;
  }
}

const glm::vec3& PlayerController::position() const noexcept {
  return position_;
}

const glm::vec3& PlayerController::facingDirection() const noexcept {
  return facingDirection_;
}

float PlayerController::yawRadians() const noexcept { return yawRadians_; }

bool PlayerController::isMoving() const noexcept { return moving_; }

float PlayerController::walkCyclePhase() const noexcept {
  return walkCyclePhase_;
}

float PlayerController::moveSpeed() const noexcept { return moveSpeed_; }

bool PlayerController::setPosition(const glm::vec3& metadata,
                                   const world::ArenaWorld& world) {
  if (!std::isfinite(metadata.y)) return false;
  const glm::vec3 position(metadata.x, gameplay::kActorCentreY, metadata.z);
  if (!world.canOccupy(position, collisionHalfExtents())) {
    return false;
  }
  position_ = position;
  moving_ = false;
  walkCyclePhase_ = 0.0F;
  return true;
}

void PlayerController::setMoveSpeed(float blocksPerSecond) {
  if (blocksPerSecond <= 0.0F || !std::isfinite(blocksPerSecond)) {
    throw std::invalid_argument(
        "Player move speed must be a finite positive value");
  }
  moveSpeed_ = blocksPerSecond;
}

const glm::vec3& PlayerController::collisionHalfExtents() noexcept {
  return kPlayerCollisionHalfExtents;
}

}  // namespace craft::app

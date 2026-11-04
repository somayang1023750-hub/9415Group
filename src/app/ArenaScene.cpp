#include "app/ArenaScene.h"

#include <cmath>
#include <stdexcept>

#include "graphics/Shader.h"

namespace craft::app {

ArenaScene::ArenaScene(const std::filesystem::path& playerTexturePath)
    : renderer_(world_, playerTexturePath) {
  if (!player_.setPosition(world_.playerSpawn(), world_)) {
    throw std::logic_error("Arena spawn cannot accommodate the player");
  }
}

void ArenaScene::resetGameplay() {
  gameplay_.reset();
  if (!player_.setPosition(world_.playerSpawn(), world_))
    throw std::logic_error("Edited map blocks the player spawn");
}

void ArenaScene::update(const MovementInput& input, float deltaSeconds,
                        std::optional<glm::vec3> shotTarget) {
  if (gameplay_.stats().phase != gameplay::Phase::Playing) return;
  const auto previousPosition = player_.position();
  player_.update(input, deltaSeconds, camera_.horizontalForward(),
                 camera_.horizontalRight(), world_);
  // Dynamic actor collision complements B's solid-block collision.
  for (const auto& enemy : gameplay_.enemies()) {
    const auto delta = player_.position() - enemy.position;
    if (std::abs(delta.x) < 0.77F && std::abs(delta.z) < 0.77F) {
      (void)player_.setPosition(previousPosition, world_);
      break;
    }
  }
  gameplay_.update(deltaSeconds, player_.position(), world_, shotTarget);
}

void ArenaScene::render(const graphics::Shader& shader, int framebufferWidth,
                        int framebufferHeight) const {
  const glm::mat4 viewProjection = camera_.viewProjection(
      player_.position(), framebufferWidth, framebufferHeight);
  renderer_.drawWorld(shader, viewProjection);
  renderer_.drawPlayer(shader, viewProjection, player_.position(),
                       player_.yawRadians(), player_.walkCyclePhase(),
                       player_.isMoving());
  for (const auto& enemy : gameplay_.enemies())
    renderer_.drawProxy(shader, viewProjection, enemy.position,
                        gameplay::Gameplay::enemyHalfExtents(),
                        {0.9F, 0.2F, 0.2F});
  for (const auto& bullet : gameplay_.bullets())
    renderer_.drawProxy(shader, viewProjection, bullet.position,
                        glm::vec3(0.10F), {1.0F, 0.9F, 0.2F});
  for (const auto& orb : gameplay_.orbs())
    renderer_.drawProxy(shader, viewProjection, orb.position, glm::vec3(0.14F),
                        {0.2F, 1.0F, 0.3F});
}

void ArenaScene::rebuildWorldGeometry() { renderer_.rebuildWorldMesh(world_); }

world::ArenaWorld& ArenaScene::world() noexcept { return world_; }

const world::ArenaWorld& ArenaScene::world() const noexcept { return world_; }

PlayerController& ArenaScene::player() noexcept { return player_; }

const PlayerController& ArenaScene::player() const noexcept { return player_; }

const FollowCamera& ArenaScene::camera() const noexcept { return camera_; }

int ArenaScene::worldVertexCount() const noexcept {
  return renderer_.worldVertexCount();
}

}  // namespace craft::app

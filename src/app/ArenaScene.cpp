#include "app/ArenaScene.h"

#include <stdexcept>

#include "graphics/Shader.h"

namespace craft::app {

ArenaScene::ArenaScene(const std::filesystem::path& playerTexturePath)
    : renderer_(world_, playerTexturePath) {
  if (!player_.setPosition(world_.playerSpawn(), world_)) {
    throw std::logic_error("Arena spawn cannot accommodate the player");
  }
}

void ArenaScene::update(const MovementInput& input, float deltaSeconds) {
  player_.update(input, deltaSeconds, camera_.horizontalForward(),
                 camera_.horizontalRight(), world_);
}

void ArenaScene::render(const graphics::Shader& shader, int framebufferWidth,
                        int framebufferHeight) const {
  const glm::mat4 viewProjection = camera_.viewProjection(
      player_.position(), framebufferWidth, framebufferHeight);
  renderer_.drawWorld(shader, viewProjection);
  renderer_.drawPlayer(shader, viewProjection, player_.position(),
                       player_.yawRadians(), player_.walkCyclePhase(),
                       player_.isMoving());
}

void ArenaScene::rebuildWorldGeometry() {
  renderer_.rebuildWorldMesh(world_);
}

world::ArenaWorld& ArenaScene::world() noexcept {
  return world_;
}

const world::ArenaWorld& ArenaScene::world() const noexcept {
  return world_;
}

PlayerController& ArenaScene::player() noexcept {
  return player_;
}

const PlayerController& ArenaScene::player() const noexcept {
  return player_;
}

const FollowCamera& ArenaScene::camera() const noexcept {
  return camera_;
}

int ArenaScene::worldVertexCount() const noexcept {
  return renderer_.worldVertexCount();
}

}  // namespace craft::app

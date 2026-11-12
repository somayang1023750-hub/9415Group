#include "app/ArenaScene.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/geometric.hpp>
#include <stdexcept>

#include "core/gameplay/Aim.h"
#include "graphics/Shader.h"

namespace craft::app {

ArenaScene::ArenaScene(const std::filesystem::path& playerTexturePath,
                       const std::filesystem::path& zombieTexturePath,
                       const std::filesystem::path& sandTexturePath)
    : renderer_(world_, playerTexturePath, zombieTexturePath,
                sandTexturePath) {
  if (!player_.setPosition(world_.playerSpawn(), world_)) {
    throw std::logic_error("Arena spawn cannot accommodate the player");
  }
}

void ArenaScene::selectLevel(world::ArenaLevel level) {
  if (level == level_) return;
  world_ = world::ArenaWorld(world::makeArenaLayout(level));
  rebuildWorldGeometry();
  level_ = level;
  resetGameplay();
}

void ArenaScene::resetGameplay() {
  gameplay_.reset(level_);
  effects_.reset();
  enemyRenderStates_.clear();
  if (!player_.setPosition(world_.playerSpawn(), world_))
    throw std::logic_error("Edited map blocks the player spawn");
}

void ArenaScene::update(const MovementInput& input, float deltaSeconds,
                        std::optional<glm::vec3> shotTarget) {
  updateInternal(input, deltaSeconds, shotTarget, nullptr);
}

void ArenaScene::updateFromMouse(const MovementInput& input,
                                float deltaSeconds,
                                const MouseAimInput& aimInput) {
  updateInternal(input, deltaSeconds, std::nullopt, &aimInput);
}

void ArenaScene::updateInternal(const MovementInput& input, float deltaSeconds,
                                std::optional<glm::vec3> shotTarget,
                                const MouseAimInput* aimInput) {
  const bool simulationUpdated =
      gameplay_.stats().phase == gameplay::Phase::Playing;
  if (simulationUpdated) {
    const auto previousPosition = player_.position();
    player_.update(input, deltaSeconds, camera_.horizontalForward(),
                   camera_.horizontalRight(), world_);
    // Dynamic actor collision complements B's solid-block collision.
    for (const auto& enemy : gameplay_.enemies()) {
      const auto delta = player_.position() - enemy.position;
      const auto separation = gameplay::playerHalfExtents() +
                              gameplay::enemyProfile(enemy.kind).halfExtents;
      if (std::abs(delta.x) < separation.x && std::abs(delta.z) < separation.z) {
        (void)player_.setPosition(previousPosition, world_);
        break;
      }
    }
    if (aimInput && aimInput->framebufferWidth > 0 &&
        aimInput->framebufferHeight > 0) {
      const glm::mat4 viewProjection = camera_.viewProjection(
          player_.position(), aimInput->framebufferWidth,
          aimInput->framebufferHeight);
      shotTarget =
          gameplay::aimOnPlane(viewProjection, aimInput->x, aimInput->y,
                               aimInput->windowWidth, aimInput->windowHeight);
    }
    gameplay_.update(deltaSeconds, player_.position(), world_, shotTarget);
    effects_.update(deltaSeconds, gameplay_.impacts(),
                    gameplay_.burningAreas(), gameplay_.bullets());
  }

  const auto& enemies = gameplay_.enemies();
  for (const auto& enemy : enemies) {
    auto [stateIt, inserted] = enemyRenderStates_.try_emplace(enemy.id);
    EnemyRenderState& state = stateIt->second;
    if (inserted) {
      state.previousPosition = enemy.position;
      glm::vec3 facing = enemy.waypoint.value_or(player_.position()) -
                         enemy.position;
      facing.y = 0.0F;
      if (glm::length(facing) > 0.0001F)
        state.yawRadians = std::atan2(facing.x, facing.z);
      continue;
    }

    glm::vec3 moved = enemy.position - state.previousPosition;
    moved.y = 0.0F;
    state.moving = glm::length(moved) > 0.00001F;
    if (state.moving) {
      state.yawRadians = std::atan2(moved.x, moved.z);
      const int currentCellX =
          static_cast<int>(std::floor(enemy.position.x));
      const int currentCellZ =
          static_cast<int>(std::floor(enemy.position.z));
      const float terrainSpeed =
          world_.isWaterSurface(currentCellX, currentCellZ)
              ? world::ArenaWorld::kWaterMovementSpeedMultiplier
              : 1.0F;
      state.walkCyclePhase = std::fmod(
          state.walkCyclePhase +
              std::clamp(deltaSeconds, 0.0F, 0.1F) * 10.0F * terrainSpeed *
                  gameplay::enemyProfile(enemy.kind).speed / gameplay::Gameplay::enemySpeed(),
          6.2831853F);
    }
    if (state.attackAnimationTime >= 0.0F) {
      state.attackAnimationTime += std::clamp(deltaSeconds, 0.0F, 0.1F);
      if (state.attackAnimationTime >=
          graphics::ArenaRenderer::kZombieAttackAnimationSeconds)
        state.attackAnimationTime = -1.0F;
    }
    state.previousPosition = enemy.position;
  }

  for (auto stateIt = enemyRenderStates_.begin();
       stateIt != enemyRenderStates_.end();) {
    const bool stillAlive = std::any_of(
        enemies.begin(), enemies.end(), [&](const gameplay::Enemy& enemy) {
          return enemy.id == stateIt->first;
        });
    if (stillAlive)
      ++stateIt;
    else
      stateIt = enemyRenderStates_.erase(stateIt);
  }

  if (simulationUpdated) {
    for (const auto& attack : gameplay_.attacks()) {
      const auto stateIt = enemyRenderStates_.find(attack.enemyId);
      if (stateIt != enemyRenderStates_.end()) {
        stateIt->second.attackAnimationTime = 0.0F;
        stateIt->second.yawRadians =
            std::atan2(attack.direction.x, attack.direction.z);
      }
    }
  }
}

void ArenaScene::render(const graphics::Shader& shader, int framebufferWidth,
                        int framebufferHeight, bool normalMapping) const {
  const glm::mat4 viewProjection = camera_.viewProjection(
      player_.position(), framebufferWidth, framebufferHeight);
  renderer_.setEffectLights(shader, level_ == world::ArenaLevel::Night,
                            effects_.lights());
  renderer_.drawWorld(shader, viewProjection,
                      camera_.worldPosition(player_.position()), level_,
                      player_.position(), normalMapping);
  renderer_.drawPlayer(shader, viewProjection, player_.position(),
                       player_.yawRadians(), player_.walkCyclePhase(),
                       player_.isMoving());
  std::vector<graphics::ArenaRenderer::ZombieInstance> zombieInstances;
  zombieInstances.reserve(gameplay_.enemies().size());
  for (const auto& enemy : gameplay_.enemies()) {
    float yawRadians = 0.0F;
    float walkCyclePhase = 0.0F;
    float attackAnimationTime = -1.0F;
    bool moving = false;
    const auto stateIt = enemyRenderStates_.find(enemy.id);
    if (stateIt != enemyRenderStates_.end()) {
      yawRadians = stateIt->second.yawRadians;
      walkCyclePhase = stateIt->second.walkCyclePhase;
      attackAnimationTime = stateIt->second.attackAnimationTime;
      moving = stateIt->second.moving;
    } else {
      glm::vec3 facing = enemy.waypoint.value_or(player_.position()) -
                         enemy.position;
      facing.y = 0.0F;
      if (glm::length(facing) > 0.0001F)
        yawRadians = std::atan2(facing.x, facing.z);
    }
    zombieInstances.push_back({enemy.position, yawRadians, walkCyclePhase,
                               moving, attackAnimationTime, enemy.kind});
  }
  renderer_.drawZombies(shader, viewProjection, zombieInstances);
  for (const auto& bullet : gameplay_.bullets()) {
    const bool laser = bullet.weapon == gameplay::Weapon::Laser;
    const bool bomb = bullet.weapon == gameplay::Weapon::Bomb;
    const glm::vec3 bulletScale =
        laser ? glm::vec3(0.06F, 0.06F, 0.55F)
        : bomb ? glm::vec3(0.22F)
               : glm::vec3(bullet.renderSize);
    renderer_.drawProxy(shader, viewProjection, bullet.position,
                        bulletScale,
                        laser  ? (bullet.evolved ? glm::vec3(1.0F, 0.08F, 0.12F)
                                                 : glm::vec3(0.1F, 1.F, 1.F))
                        : bomb ? (bullet.evolved ? glm::vec3(1.F, 0.12F, 0.06F)
                                                 : glm::vec3(1.F, 0.4F, 0.1F))
                               : glm::vec3(1.F, 0.9F, 0.2F),
                        std::atan2(bullet.velocity.x, bullet.velocity.z), true);
  }
  for (const auto& blast : gameplay_.explosions())
    renderer_.drawBlast(shader, viewProjection, blast.position, blast.radius,
                        blast.age);
  for (const auto& orb : gameplay_.orbs())
    renderer_.drawProxy(
        shader, viewProjection, orb.position,
        glm::vec3(orb.sourceKind == gameplay::EnemyKind::Golem ? .25F : .14F),
        {0.2F, 1.0F, 0.3F});
  renderer_.drawParticles(shader, viewProjection, effects_.visuals());
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

int ArenaScene::worldDrawCallCount() const noexcept {
  return renderer_.worldDrawCallCount();
}

}  // namespace craft::app

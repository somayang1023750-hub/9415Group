#pragma once

#include <array>
#include <glm/vec3.hpp>

#include "core/gameplay/ActorGeometry.h"
#include "world/ArenaLayout.h"

namespace craft::gameplay {
enum class EnemyKind { Zombie, Golem, Spider };
struct EnemyProfile {
  float health;
  int contactDamage;
  int experience;
  float speed;
  glm::vec3 halfExtents;
  float centreY;
};
// A/C contract: use the actual type bounds for spawn, pathfinding, hits and
// models. Spider HP is 4 * 0.75 = 3; profiles remain floating point.
inline EnemyProfile enemyProfile(EnemyKind kind) {
  switch (kind) {
    case EnemyKind::Golem:
      return {14.F, 2, 2, 2.1F, {.37F, .43F, .37F}, 1.442F};
    case EnemyKind::Spider:
      return {4.F * .75F, 1, 1, 4.5F, {.22F, .20F, .24F}, 1.212F};
    default:
      return {4.F, 1, 1, 3.3F, enemyHalfExtents(), kActorCentreY};
  }
}
// Order: zombie, golem, spider. Weights describe generated enemies, not live
// population (kills affect the latter). Ramps use active gameplay seconds.
inline std::array<float, 3> enemySpawnWeights(world::ArenaLevel level,
                                              float elapsedSeconds) {
  const auto ramp = [](float t, float start) {
    return t <= start ? 0.F : t >= start + 60.F ? 1.F : (t - start) / 60.F;
  };
  if (level == world::ArenaLevel::Day) {
    const float golem = .30F * ramp(elapsedSeconds, 60.F);
    return {1.F - golem, golem, 0.F};
  }
  // Night starts with Spiders; new types join smoothly after their release.
  const float zombie = .50F * ramp(elapsedSeconds, 30.F);
  const float golem = .20F * ramp(elapsedSeconds, 60.F);
  return {zombie, golem, 1.F - zombie - golem};
}
inline float enemySpawnInterval(world::ArenaLevel level, float elapsedSeconds) {
  if (level == world::ArenaLevel::Day) return 2.F;
  if (elapsedSeconds <= 30.F) return 1.5F;
  if (elapsedSeconds >= 60.F) return 2.F;
  // Gradually slow Night batches from 1.5 s to 2 s over 30-60 active seconds.
  return 1.5F + (elapsedSeconds - 30.F) / 60.F;
}

inline const char* enemyName(EnemyKind kind) {
  switch (kind) {
    case EnemyKind::Golem:
      return "Golem";
    case EnemyKind::Spider:
      return "Spider";
    default:
      return "Zombie";
  }
}
}  // namespace craft::gameplay

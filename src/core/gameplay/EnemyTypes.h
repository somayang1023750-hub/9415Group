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
// models. Spider HP remains exactly 4 * 0.85, rather than rounding to an int.
inline EnemyProfile enemyProfile(EnemyKind kind) {
  switch (kind) {
    case EnemyKind::Golem:
      return {14.F, 2, 2, 2.1F, {.37F, .43F, .37F}, 1.442F};
    case EnemyKind::Spider:
      return {4.F * .85F, 1, 1, 4.5F, {.22F, .20F, .24F}, 1.212F};
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
  const float golem = level == world::ArenaLevel::Day
                          ? .30F * ramp(elapsedSeconds, 60.F)
                          : .15F * ramp(elapsedSeconds, 30.F);
  const float spider = level == world::ArenaLevel::Night
                           ? .25F * ramp(elapsedSeconds, 60.F)
                           : 0.F;
  return {1.F - golem - spider, golem, spider};
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

#pragma once

#include <glm/vec3.hpp>

namespace craft::gameplay {
// Shared grounded actor proportions for A collision and C rendering/aiming.
// B's layout spawn metadata remains unchanged; consumers use its X/Z and place
// the smaller actor on the existing floor (top Y=1).
inline constexpr float kActorScale = 0.70F;
inline constexpr float kActorCentreY = 1.32F;
inline glm::vec3 playerHalfExtents() { return {0.294F, 0.308F, 0.294F}; }
inline glm::vec3 enemyHalfExtents() { return {0.245F, 0.308F, 0.245F}; }
inline float playerEnemySeparation() {
  return playerHalfExtents().x + enemyHalfExtents().x;
}
inline float enemySeparation() { return 2 * enemyHalfExtents().x; }
}  // namespace craft::gameplay

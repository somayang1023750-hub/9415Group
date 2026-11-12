#pragma once
#include <cmath>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <optional>

#include "core/gameplay/ActorGeometry.h"
namespace craft::gameplay {
// Unproject the logical window cursor through the same view-projection matrix
// used to render the scene, then intersect that ray with the player's height
// plane. Gameplay fires horizontally from the player toward the returned X/Z.
inline std::optional<glm::vec3> aimOnPlane(const glm::mat4& vp, double x,
                                           double y, int width, int height,
                                           float planeY = kActorCentreY) {
  if (width <= 0 || height <= 0 || x < 0 || y < 0 || x > width || y > height)
    return std::nullopt;
  const auto inv = glm::inverse(vp);
  auto a = inv * glm::vec4(2 * x / width - 1, 1 - 2 * y / height, -1, 1);
  auto b = inv * glm::vec4(2 * x / width - 1, 1 - 2 * y / height, 1, 1);
  a /= a.w;
  b /= b.w;
  const float dy = b.y - a.y;
  if (!std::isfinite(dy) || std::abs(dy) < 1e-6F) return std::nullopt;
  const float t = (planeY - a.y) / dy;
  if (t < 0 || t > 1) return std::nullopt;
  return glm::vec3(a + (b - a) * t);
}
}  // namespace craft::gameplay

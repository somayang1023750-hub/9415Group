#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace craft::app {

// Orthographic camera about 60 degrees above the arena plane.
// Fixed local zoom follows the player and clips the distant arena.
class FollowCamera {
 public:
  [[nodiscard]] glm::vec3 worldPosition(const glm::vec3& target) const noexcept;
  [[nodiscard]] glm::vec3 horizontalForward() const noexcept;
  [[nodiscard]] glm::vec3 horizontalRight() const noexcept;
  [[nodiscard]] glm::mat4 viewProjection(const glm::vec3& target,
                                         int framebufferWidth,
                                         int framebufferHeight) const;
};

}  // namespace craft::app

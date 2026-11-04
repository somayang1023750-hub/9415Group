#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace craft::app {

// Fixed diagonal orthographic camera. Its position tracks the player while
// its projection keeps a stable scale as the player moves.
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

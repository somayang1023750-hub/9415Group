#include "app/FollowCamera.h"

#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <stdexcept>

namespace craft::app {

namespace {

const glm::vec3 kWorldUp{0.0F, 1.0F, 0.0F};
constexpr float kHorizontalCameraDistance = 7.0F;
constexpr float kVerticalCameraDistance = 26.0F;
constexpr float kHalfHeight = 8.0F;
constexpr float kNearPlane = 0.1F;
constexpr float kFarPlane = 80.0F;

}  // namespace

glm::vec3 FollowCamera::horizontalForward() const noexcept {
  return glm::normalize(glm::vec3{-1.0F, 0.0F, -1.0F});
}

glm::vec3 FollowCamera::worldPosition(const glm::vec3& target) const noexcept {
  constexpr float inverseSqrtTwo = 0.70710678118F;
  return target + glm::vec3{kHorizontalCameraDistance * inverseSqrtTwo,
                            kVerticalCameraDistance,
                            kHorizontalCameraDistance * inverseSqrtTwo};
}

glm::vec3 FollowCamera::horizontalRight() const noexcept {
  return glm::normalize(glm::cross(horizontalForward(), kWorldUp));
}

glm::mat4 FollowCamera::viewProjection(const glm::vec3& target,
                                       int framebufferWidth,
                                       int framebufferHeight) const {
  if (framebufferWidth <= 0 || framebufferHeight <= 0) {
    throw std::invalid_argument(
        "FollowCamera requires a non-empty framebuffer");
  }

  const glm::vec3 cameraPosition = worldPosition(target);
  const glm::mat4 view = glm::lookAt(cameraPosition, target, kWorldUp);

  // Local player-centred view, about 15 degrees from vertical. Never fit the
  // whole arena: fixed zoom and normal clipping keep distant areas off screen.
  // C: tune kHalfHeight/offsets together; aiming uses this exact same matrix.
  const float aspect = static_cast<float>(framebufferWidth) /
                       static_cast<float>(framebufferHeight);
  const float halfWidth = kHalfHeight * aspect;
  const glm::mat4 projection = glm::ortho(-halfWidth, halfWidth, -kHalfHeight,
                                          kHalfHeight, kNearPlane, kFarPlane);
  return projection * view;
}

}  // namespace craft::app

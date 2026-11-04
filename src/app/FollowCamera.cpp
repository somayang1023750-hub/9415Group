#include "app/FollowCamera.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

namespace craft::app {

namespace {

const glm::vec3 kWorldUp{0.0F, 1.0F, 0.0F};
constexpr float kHorizontalCameraDistance = 24.0F;
constexpr float kVerticalCameraDistance = 28.0F;
constexpr float kArenaCenter = 16.0F;
constexpr float kPlayerEyeHeight = 1.45F;
constexpr float kMinimumHalfHeight = 18.0F;
constexpr float kFrameMargin = 2.0F;
constexpr float kNearPlane = 0.1F;
constexpr float kFarPlane = 160.0F;

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

  // Keep the camera's scale fixed as it follows the player. Derive the
  // projection from a stable view at the arena's starting focus; measuring
  // bounds against the moving view makes the orthographic volume grow near
  // the arena edges and looks like the camera is zooming out.
  const glm::vec3 framingTarget{kArenaCenter, kPlayerEyeHeight, kArenaCenter};
  const glm::vec3 framingCameraPosition = worldPosition(framingTarget);
  const glm::mat4 framingView =
      glm::lookAt(framingCameraPosition, framingTarget, kWorldUp);

  const float aspect = static_cast<float>(framebufferWidth) /
                       static_cast<float>(framebufferHeight);
  float halfHeight = kMinimumHalfHeight;
  float halfWidth = kMinimumHalfHeight * aspect;

  // Frame the playable arena footprint, including its low perimeter walls.
  constexpr float kArenaTop = 3.0F;
  const std::array<glm::vec3, 8> arenaCorners{{
      {0.0F, 0.0F, 0.0F},
      {32.0F, 0.0F, 0.0F},
      {0.0F, 0.0F, 32.0F},
      {32.0F, 0.0F, 32.0F},
      {0.0F, kArenaTop, 0.0F},
      {32.0F, kArenaTop, 0.0F},
      {0.0F, kArenaTop, 32.0F},
      {32.0F, kArenaTop, 32.0F},
  }};

  for (const glm::vec3& corner : arenaCorners) {
    const glm::vec4 viewCorner = framingView * glm::vec4{corner, 1.0F};
    halfWidth = std::max(halfWidth, std::abs(viewCorner.x) + kFrameMargin);
    halfHeight =
        std::max(halfHeight, std::abs(viewCorner.y) + kFrameMargin);
  }

  // Keep a consistent aspect ratio while satisfying both projected bounds.
  halfHeight = std::max(halfHeight, halfWidth / aspect);
  halfWidth = halfHeight * aspect;

  const glm::mat4 projection =
      glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, kNearPlane,
                 kFarPlane);
  return projection * view;
}

}  // namespace craft::app

#pragma once

#include <optional>

#include <glm/vec3.hpp>

#include "world/ArenaLayout.h"

namespace craft::app {

// UI actions are requests for the gameplay layer to handle. This UI-only
// starter emits them but intentionally does not mutate the arena or game state.
enum class GameUiAction {
  RestartCurrentConfiguration,
  SelectLevel1,
  SelectLevel2,
};

struct UiDiagnostics {
  glm::vec3 cameraPosition{0.0F};
  glm::vec3 playerPosition{0.0F};
  int renderedWorldFaces = 0;
  int renderedTriangles = 0;
  int drawCalls = 0;
  int framebufferWidth = 0;
  int framebufferHeight = 0;
  float frameTimeMilliseconds = 0.0F;
};

// Student-facing extension point for gameplay, HUD, and debug controls.
// It deliberately has no access to SceneBenchmark internals.
class CustomUi {
 public:
  // Draws tutorial and menu controls inside an existing ImGui window. The
  // returned action is a request only; no game behavior is performed here.
  [[nodiscard]] std::optional<GameUiAction> drawContents(
      bool interactionDisabled, world::ArenaLevel level);

  // Read-only diagnostics live in a separate always-visible debug window.
  void drawDebugPanel(const UiDiagnostics& diagnostics) const;
};

}  // namespace craft::app

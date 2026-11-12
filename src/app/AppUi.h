#pragma once

#include <optional>

#include "app/CustomUi.h"
#include "app/SceneBenchmark.h"

namespace craft::graphics {
class Window;
}

namespace craft::app {

struct AppUiFrameResult {
  bool startBenchmarkRequested = false;
  bool normalMapping = true;
  std::optional<GameUiAction> gameAction;
};

// Owns the Dear ImGui lifecycle and composes game controls, diagnostics, and
// the course-provided benchmark UI. Assignment-specific UI stays in CustomUi.
class AppUi {
 public:
  explicit AppUi(graphics::Window& window);
  ~AppUi();

  AppUi(const AppUi&) = delete;
  AppUi& operator=(const AppUi&) = delete;

  void beginFrame();
  // Returns benchmark and gameplay UI requests for the caller to handle.
  // Gameplay requests are intentionally not applied by this UI-only layer.
  [[nodiscard]] AppUiFrameResult draw(const BenchmarkSnapshot& benchmark,
                                      const UiDiagnostics& diagnostics,
                                      world::ArenaLevel level);
  void render();

 private:
  [[nodiscard]] bool drawBenchmarkSection(
      const BenchmarkSnapshot& benchmark) const;

  CustomUi customUi_;
  bool normalMapping_ = true;
};

}  // namespace craft::app

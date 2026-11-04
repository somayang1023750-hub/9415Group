#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "app/AppUi.h"
#include "app/ArenaScene.h"
#include "app/SceneBenchmark.h"
#include "app/TitleScreen.h"
#include "core/AssetPath.h"
#include "graphics/ArenaRenderer.h"
#include "graphics/OpenGL.h"
#include "graphics/Shader.h"
#include "graphics/Window.h"

namespace {

constexpr int kInitialWidth = 1280;
constexpr int kInitialHeight = 720;
constexpr const char* kSceneWorkload = "milestone1-voxel-arena";

craft::app::MovementInput readMovementInput(GLFWwindow* window) {
  return {glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS,
          glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS,
          glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS,
          glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS};
}

void startBenchmark(craft::graphics::Window& window,
                    craft::app::SceneBenchmark& benchmark) {
  window.setFramebufferSize(craft::app::kBenchmarkFramebufferWidth,
                            craft::app::kBenchmarkFramebufferHeight);
  window.setVSync(false);
  benchmark.start();
}

void printBenchmarkResult(const craft::app::BenchmarkSnapshot& result,
                          int framebufferWidth, int framebufferHeight) {
  std::cout << std::fixed << std::setprecision(4)
            << "benchmark_result sample_count=" << result.measuredFrames
            << " average_ms=" << result.averageMilliseconds
            << " average_fps=" << result.averageFramesPerSecond
            << " resolution=" << framebufferWidth << 'x' << framebufferHeight
            << " workload=" << kSceneWorkload << '\n'
            << "benchmark_note=Milestone 1 map; compare optimizations using "
               "the same layout, camera, and framebuffer size\n";
}

}  // namespace

int main() {
  try {
    craft::graphics::Window window(kInitialWidth, kInitialHeight,
                                   "Voxel Arena");
    craft::graphics::Shader shader(
        craft::core::assetPath("shaders/basic.vert"),
        craft::core::assetPath("shaders/basic.frag"));
    craft::app::ArenaScene arenaScene(
        craft::core::assetPath("textures/player.png"));
    craft::app::AppUi appUi(window);
    craft::app::SceneBenchmark benchmark;

    const bool commandLineBenchmark =
        craft::app::benchmarkRequestedFromEnvironment();
    if (commandLineBenchmark) {
      startBenchmark(window, benchmark);
    }

    // The start screen is skipped when benchmarking from the command line.
    std::optional<craft::app::TitleScreen> titleScreen;
    if (!commandLineBenchmark) {
      titleScreen.emplace(window);
    }

    glEnable(GL_DEPTH_TEST);
    double lastFrameTime = glfwGetTime();

    while (!window.shouldClose()) {
      window.pollEvents();

      const double frameTime = glfwGetTime();
      const float deltaSeconds = static_cast<float>(
          std::clamp(frameTime - lastFrameTime, 0.0, 0.1));
      lastFrameTime = frameTime;

      auto [framebufferWidth, framebufferHeight] = window.framebufferSize();
      if (framebufferWidth == 0 || framebufferHeight == 0) {
        window.swapBuffers();
        continue;
      }

      if (titleScreen) {
        titleScreen->update();
        if (!titleScreen->finished()) {
          titleScreen->draw(framebufferWidth, framebufferHeight);
          window.swapBuffers();
          continue;
        }
        // Free the title screen's GPU resources once the game starts.
        titleScreen.reset();
      }

      arenaScene.update(readMovementInput(window.nativeHandle()), deltaSeconds);

      appUi.beginFrame();
      const auto benchmarkState = benchmark.snapshot();
      const int worldVertexCount = arenaScene.worldVertexCount();
      craft::app::UiDiagnostics uiDiagnostics;
      uiDiagnostics.cameraPosition = arenaScene.camera().worldPosition(
          arenaScene.player().position());
      uiDiagnostics.playerPosition = arenaScene.player().position();
      uiDiagnostics.renderedWorldFaces = worldVertexCount / 6;
      uiDiagnostics.renderedTriangles =
          worldVertexCount / 3 +
          craft::graphics::ArenaRenderer::kPlayerPartCount *
              craft::graphics::ArenaRenderer::kTrianglesPerPlayerPart;
      uiDiagnostics.drawCalls =
          1 + craft::graphics::ArenaRenderer::kPlayerPartCount;
      uiDiagnostics.framebufferWidth = framebufferWidth;
      uiDiagnostics.framebufferHeight = framebufferHeight;
      uiDiagnostics.frameTimeMilliseconds = deltaSeconds * 1000.0F;

      const craft::app::AppUiFrameResult uiResult =
          appUi.draw(benchmarkState, uiDiagnostics);
      // Restart and level-selection events are exposed by AppUi for gameplay
      // integration; this UI-only starter intentionally performs no action.
      if (uiResult.startBenchmarkRequested && !benchmarkState.running) {
        startBenchmark(window, benchmark);
        const auto resizedFramebuffer = window.framebufferSize();
        framebufferWidth = resizedFramebuffer.first;
        framebufferHeight = resizedFramebuffer.second;
      }

      benchmark.beginSceneFrame();

      glViewport(0, 0, framebufferWidth, framebufferHeight);
      glClearColor(0.08F, 0.10F, 0.14F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      arenaScene.render(shader, framebufferWidth, framebufferHeight);

      const bool benchmarkCompleted = benchmark.endSceneFrame();
      if (benchmarkCompleted) {
        printBenchmarkResult(benchmark.snapshot(), framebufferWidth,
                             framebufferHeight);
      }

      // Dear ImGui and presentation remain outside the measured scene region.
      appUi.render();
      window.swapBuffers();

      if (benchmarkCompleted) {
        window.setVSync(true);
        if (commandLineBenchmark) {
          window.requestClose();
        }
      }
    }
  } catch (const std::exception& error) {
    std::cerr << "Craft failed: " << error.what() << '\n';
    return 1;
  }

  return 0;
}

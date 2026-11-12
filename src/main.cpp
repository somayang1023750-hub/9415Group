#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "app/AppUi.h"
#include "app/ArenaScene.h"
#include "app/GameplayUi.h"
#include "app/SceneBenchmark.h"
#include "app/TitleScreen.h"
#include "core/AssetPath.h"
#include "graphics/ArenaRenderer.h"
#include "graphics/OpenGL.h"
#include "graphics/Shader.h"
#include "graphics/Window.h"
#include "imgui.h"

namespace {

constexpr int kInitialWidth = 1280;
constexpr int kInitialHeight = 720;
constexpr const char* kSceneWorkload = "milestone2-local-follow-arena";

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
                          int framebufferWidth, int framebufferHeight,
                          const char* workload, craft::world::ArenaLevel level,
                          bool normalMapping) {
  std::cout
      << std::fixed << std::setprecision(4)
      << "benchmark_result sample_count=" << result.measuredFrames
      << " average_ms=" << result.averageMilliseconds
      << " average_fps=" << result.averageFramesPerSecond
      << " resolution=" << framebufferWidth << 'x' << framebufferHeight
      << " level=" << (level == craft::world::ArenaLevel::Day ? "day" : "night")
      << " normal_maps=" << (normalMapping ? "on" : "off")
      << " workload=" << workload << '\n'
      << "benchmark_note=Local follow camera; compare optimizations using "
         "the same layout, actor snapshot, camera, and framebuffer size\n";
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
        craft::core::assetPath("textures/player.png"),
        craft::core::assetPath("textures/zombie.png"),
        craft::core::assetPath("textures/sand_noise_uv.png"));
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
      const float deltaSeconds =
          static_cast<float>(std::clamp(frameTime - lastFrameTime, 0.0, 0.1));
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

      appUi.beginFrame();
      const bool mouseDown =
          glfwGetMouseButton(window.nativeHandle(), GLFW_MOUSE_BUTTON_LEFT) ==
          GLFW_PRESS;
      std::optional<craft::app::ArenaScene::MouseAimInput> mouseAim;
      if (mouseDown && !ImGui::GetIO().WantCaptureMouse) {
        double x, y;
        int w, h;
        glfwGetCursorPos(window.nativeHandle(), &x, &y);
        glfwGetWindowSize(window.nativeHandle(), &w, &h);
        mouseAim = craft::app::ArenaScene::MouseAimInput{
            x, y, w, h, framebufferWidth, framebufferHeight};
      }
      // C: the command-line benchmark freezes the current local camera scene.
      // Frozen in-game snapshots have their own label. Simulation is frozen for
      // every measured run, including command-line runs; no timing comparison
      // with dynamic gameplay is implied.
      if (!benchmark.snapshot().running) {
        const auto movementInput =
            ImGui::GetIO().WantCaptureKeyboard
                ? craft::app::MovementInput{}
                : readMovementInput(window.nativeHandle());
        if (mouseAim) {
          arenaScene.updateFromMouse(movementInput, deltaSeconds, *mouseAim);
        } else {
          arenaScene.update(movementInput, deltaSeconds);
        }
      }
      if (craft::app::drawGameplayUi(arenaScene.gameplay()))
        arenaScene.resetGameplay();
      const auto benchmarkState = benchmark.snapshot();
      const int worldVertexCount = arenaScene.worldVertexCount();
      craft::app::UiDiagnostics uiDiagnostics;
      uiDiagnostics.cameraPosition =
          arenaScene.camera().worldPosition(arenaScene.player().position());
      uiDiagnostics.playerPosition = arenaScene.player().position();
      uiDiagnostics.renderedWorldFaces = worldVertexCount / 6;
      uiDiagnostics.renderedTriangles =
          worldVertexCount / 3 +
          craft::graphics::ArenaRenderer::kPlayerPartCount *
              craft::graphics::ArenaRenderer::kTrianglesPerPlayerPart;
      int enemyParts = 0, enemyDrawCalls = 0;
      bool kinds[3] = {};
      for (const auto& enemy : arenaScene.gameplay().enemies()) {
        const int parts =
            craft::graphics::ArenaRenderer::enemyPartCount(enemy.kind);
        enemyParts += parts;
        const int kind = static_cast<int>(enemy.kind);
        if (!kinds[kind]) {
          enemyDrawCalls += parts;
          kinds[kind] = true;
        }
      }
      uiDiagnostics.renderedTriangles +=
          enemyParts * craft::graphics::ArenaRenderer::kTrianglesPerPlayerPart;
      const int proxyCount =
          static_cast<int>(arenaScene.gameplay().bullets().size() +
                           arenaScene.gameplay().orbs().size());
      uiDiagnostics.renderedTriangles +=
          (proxyCount + arenaScene.particleCount()) * 12;
      uiDiagnostics.drawCalls =
          arenaScene.worldDrawCallCount() +
          craft::graphics::ArenaRenderer::kPlayerPartCount + enemyDrawCalls +
          proxyCount + (arenaScene.particleCount() > 0 ? 1 : 0) +
          static_cast<int>(arenaScene.gameplay().explosions().size());
      uiDiagnostics.framebufferWidth = framebufferWidth;
      uiDiagnostics.framebufferHeight = framebufferHeight;
      uiDiagnostics.frameTimeMilliseconds = deltaSeconds * 1000.0F;

      const craft::app::AppUiFrameResult uiResult =
          appUi.draw(benchmarkState, uiDiagnostics, arenaScene.level());
      if (uiResult.gameAction ==
          craft::app::GameUiAction::RestartCurrentConfiguration)
        arenaScene.resetGameplay();
      else if (uiResult.gameAction == craft::app::GameUiAction::SelectLevel1)
        arenaScene.selectLevel(craft::world::ArenaLevel::Day);
      else if (uiResult.gameAction == craft::app::GameUiAction::SelectLevel2)
        arenaScene.selectLevel(craft::world::ArenaLevel::Night);
      if (uiResult.startBenchmarkRequested && !benchmarkState.running) {
        startBenchmark(window, benchmark);
        const auto resizedFramebuffer = window.framebufferSize();
        framebufferWidth = resizedFramebuffer.first;
        framebufferHeight = resizedFramebuffer.second;
      }

      benchmark.beginSceneFrame();

      glViewport(0, 0, framebufferWidth, framebufferHeight);
      if (arenaScene.level() == craft::world::ArenaLevel::Night)
        glClearColor(0.012F, 0.018F, 0.038F, 1.0F);
      else
        glClearColor(0.08F, 0.10F, 0.14F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      arenaScene.render(shader, framebufferWidth, framebufferHeight,
                        uiResult.normalMapping);

      const bool benchmarkCompleted = benchmark.endSceneFrame();
      if (benchmarkCompleted) {
        printBenchmarkResult(
            benchmark.snapshot(), framebufferWidth, framebufferHeight,
            commandLineBenchmark ? kSceneWorkload
                                 : "milestone2-local-follow-gameplay-frozen",
            arenaScene.level(), uiResult.normalMapping);
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

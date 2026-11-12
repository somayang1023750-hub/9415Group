#include "app/AppUi.h"

#include <stdexcept>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "graphics/Window.h"

namespace craft::app {

AppUi::AppUi(graphics::Window& window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForOpenGL(window.nativeHandle(), true)) {
    ImGui::DestroyContext();
    throw std::runtime_error("Failed to initialise Dear ImGui GLFW backend");
  }
  if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    throw std::runtime_error("Failed to initialise Dear ImGui OpenGL backend");
  }
}

AppUi::~AppUi() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void AppUi::beginFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

AppUiFrameResult AppUi::draw(const BenchmarkSnapshot& benchmark,
                             const UiDiagnostics& diagnostics,
                             world::ArenaLevel level) {
  constexpr float panelWidth = 330.0F;
  ImGui::SetNextWindowSize(ImVec2(panelWidth, 0.0F), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F), ImGuiCond_FirstUseEver);
  ImGui::Begin("Craft controls", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

  AppUiFrameResult result;
  result.gameAction = customUi_.drawContents(benchmark.running, level);
  ImGui::BeginDisabled(benchmark.running);
  ImGui::Checkbox("Surface normal maps", &normalMapping_);
  ImGui::EndDisabled();
  result.normalMapping = normalMapping_;
  ImGui::SeparatorText("240-frame benchmark");
  result.startBenchmarkRequested = drawBenchmarkSection(benchmark);

  ImGui::End();
  customUi_.drawDebugPanel(diagnostics);
  return result;
}

void AppUi::render() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool AppUi::drawBenchmarkSection(const BenchmarkSnapshot& benchmark) const {
  bool startBenchmark = false;

  if (benchmark.running) {
    if (benchmark.warmupFrames < benchmark.requiredWarmupFrames) {
      ImGui::Text("Warm-up: %zu / %zu", benchmark.warmupFrames,
                  benchmark.requiredWarmupFrames);
    } else {
      ImGui::Text("Measured: %zu / %zu", benchmark.measuredFrames,
                  benchmark.requiredMeasuredFrames);
      ImGui::Text("Average: %.4f ms/frame", benchmark.averageMilliseconds);
      ImGui::Text("Average FPS: %.1f", benchmark.averageFramesPerSecond);
    }
  } else {
    if (benchmark.complete) {
      ImGui::Text("Complete: %zu samples", benchmark.measuredFrames);
      ImGui::Text("Average: %.4f ms/frame", benchmark.averageMilliseconds);
      ImGui::Text("Average FPS: %.1f", benchmark.averageFramesPerSecond);
    } else {
      ImGui::TextUnformatted("Ready");
    }
    startBenchmark = ImGui::Button("Run benchmark");
  }

  return startBenchmark;
}

}  // namespace craft::app

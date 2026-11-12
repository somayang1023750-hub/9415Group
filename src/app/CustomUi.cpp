#include "app/CustomUi.h"

#include <optional>

#include "imgui.h"

namespace craft::app {

std::optional<GameUiAction> CustomUi::drawContents(
    bool interactionDisabled, world::ArenaLevel level) {
  std::optional<GameUiAction> action;

  ImGui::SeparatorText("How to play");
  ImGui::TextUnformatted("W: move forward (camera-relative)");
  ImGui::TextUnformatted("A: move left (camera-relative)");
  ImGui::TextUnformatted("S: move backward (camera-relative)");
  ImGui::TextUnformatted("D: move right (camera-relative)");
  ImGui::TextUnformatted("Mouse: aim");
  ImGui::TextUnformatted("Left mouse button: shoot");
  ImGui::TextUnformatted("ESC: exit");

  ImGui::SeparatorText("Game menu");
  ImGui::BeginDisabled(interactionDisabled);
  if (ImGui::Button("Restart current configuration", ImVec2(-1.0F, 0.0F))) {
    action = GameUiAction::RestartCurrentConfiguration;
  }

  ImGui::TextUnformatted("Switch level to start a new run");
  const bool isDay = level == world::ArenaLevel::Day;
  if (ImGui::RadioButton("Day", isDay) && !isDay)
    action = GameUiAction::SelectLevel1;
  ImGui::SameLine();
  if (ImGui::RadioButton("Night", !isDay) && isDay)
    action = GameUiAction::SelectLevel2;
  ImGui::EndDisabled();

  return action;
}

void CustomUi::drawDebugPanel(const UiDiagnostics& diagnostics) const {
  const ImGuiIO& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 310.0F, 12.0F),
                          ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(298.0F, 0.0F), ImGuiCond_Always);
  ImGui::Begin("Debug panel", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

  ImGui::SeparatorText("Camera");
  ImGui::Text("Position: %.2f, %.2f, %.2f", diagnostics.cameraPosition.x,
              diagnostics.cameraPosition.y, diagnostics.cameraPosition.z);
  ImGui::Text("Player:   %.2f, %.2f, %.2f", diagnostics.playerPosition.x,
              diagnostics.playerPosition.y, diagnostics.playerPosition.z);

  ImGui::SeparatorText("Geometry");
  ImGui::Text("World mesh faces: %d", diagnostics.renderedWorldFaces);
  ImGui::Text("Triangles (incl. player): %d", diagnostics.renderedTriangles);
  ImGui::Text("Scene draw calls: %d", diagnostics.drawCalls);

  ImGui::SeparatorText("Frame");
  ImGui::Text("Frame time: %.2f ms", diagnostics.frameTimeMilliseconds);
  const float fps = diagnostics.frameTimeMilliseconds > 0.0F
                        ? 1000.0F / diagnostics.frameTimeMilliseconds
                        : 0.0F;
  ImGui::Text("FPS: %.1f", fps);
  ImGui::Text("Framebuffer: %d x %d", diagnostics.framebufferWidth,
              diagnostics.framebufferHeight);

  ImGui::End();
}

}  // namespace craft::app

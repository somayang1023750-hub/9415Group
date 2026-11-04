#include "app/GameplayUi.h"

#include "core/gameplay/Gameplay.h"
#include "imgui.h"
namespace craft::app {
void drawGameplayUi(gameplay::Gameplay& game) {
  const auto& s = game.stats();
  ImGui::SetNextWindowPos(ImVec2(12, 540), ImGuiCond_FirstUseEver);
  ImGui::Begin("Arena status");
  ImGui::Text("HP %d / 5 | Time %.1f | Kills %d", s.health, s.remainingSeconds,
              s.kills);
  ImGui::Text("Level %d | EXP %d / %d", s.level, s.experience,
              game.experienceToNextLevel());
  ImGui::Text("Weapon: %s", gameplay::weaponName(s.weapon));
  if (s.phase == gameplay::Phase::ChoosingUpgrade) {
    ImGui::TextUnformatted(
        "Level up: choose a random weapon offer (game paused)");
    const auto choices = game.upgradeChoices();
    for (std::size_t i = 0; i < choices.size(); ++i) {
      ImGui::PushID(static_cast<int>(i));
      if (ImGui::Button(gameplay::weaponName(choices[i])))
        game.chooseUpgrade(i);
      ImGui::PopID();
    }
  }
  if (s.phase == gameplay::Phase::Won)
    ImGui::TextUnformatted("Survived! Restart from Game menu.");
  if (s.phase == gameplay::Phase::Lost)
    ImGui::TextUnformatted("Defeated. Restart from Game menu.");
  ImGui::End();
}
}  // namespace craft::app

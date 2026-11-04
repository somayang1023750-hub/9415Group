#include "app/GameplayUi.h"

#include <algorithm>

#include "core/gameplay/Gameplay.h"
#include "imgui.h"
namespace craft::app {
void drawGameplayUi(gameplay::Gameplay& game) {
  const auto& s = game.stats();
  ImGui::SetNextWindowPos(ImVec2(12, 540), ImGuiCond_FirstUseEver);
  ImGui::Begin("Arena status", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("HP %d / 5 | Time %.1f | Kills %d", s.health, s.remainingSeconds,
              s.kills);
  ImGui::Text("Level %d | EXP %d / %d", s.level, s.experience,
              game.experienceToNextLevel());
  ImGui::Text("Weapon: %s", gameplay::weaponName(s.weapon));
  const auto equipped = gameplay::weaponProfile(s.weapon);
  ImGui::Text("Damage %d | Shot interval %.2fs | Bullet speed %.0f",
              equipped.baseDamage + s.level - 1, equipped.shotCooldown,
              equipped.bulletSpeed);
  if (s.phase == gameplay::Phase::ChoosingUpgrade)
    ImGui::TextUnformatted("LEVEL UP - choose a weapon in the centre dialog.");
  if (s.phase == gameplay::Phase::Won)
    ImGui::TextUnformatted("Survived! Restart from Game menu.");
  if (s.phase == gameplay::Phase::Lost)
    ImGui::TextUnformatted("Defeated. Restart from Game menu.");
  ImGui::End();

  // C: this temporary modal owns presentation only. Keep chooseUpgrade as the
  // resume boundary when replacing it with the final level-up menu. It lives
  // outside Arena status so saved/collapsed HUD sizes cannot hide the choices.
  if (s.phase != gameplay::Phase::ChoosingUpgrade) return;
  constexpr const char* popup = "LEVEL UP - Choose your weapon";
  if (!ImGui::IsPopupOpen(popup)) ImGui::OpenPopup(popup);
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const float width =
      std::max(1.0F, std::min(520.0F, viewport->WorkSize.x - 32));
  ImGui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_Always,
                          ImVec2(0.5F, 0.5F));
  ImGui::SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Always);
  ImGui::SetNextWindowSizeConstraints(
      ImVec2(width, 0),
      ImVec2(width, std::max(1.0F, viewport->WorkSize.y - 32)));
  // No close button: dismissing without a valid selection would leave the
  // simulation paused. Modal input also prevents clicks reaching the arena.
  if (ImGui::BeginPopupModal(popup, nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings)) {
    ImGui::Text("Level %d complete!", s.level);
    ImGui::TextWrapped("GAME PAUSED - enemies and the countdown are stopped.");
    ImGui::TextWrapped(
        "Choose one weapon below to upgrade and resume playing.");
    ImGui::Separator();
    const auto choices = game.upgradeChoices();
    for (std::size_t i = 0; i < choices.size(); ++i) {
      ImGui::PushID(static_cast<int>(i));
      if (ImGui::Button(gameplay::weaponName(choices[i]), ImVec2(-1, 44))) {
        if (game.chooseUpgrade(i)) ImGui::CloseCurrentPopup();
        ImGui::PopID();
        break;
      }
      const auto profile = gameplay::weaponProfile(choices[i]);
      ImGui::Text("After upgrade: %d damage | %.2fs shot interval",
                  profile.baseDamage + s.level, profile.shotCooldown);
      ImGui::Text("Bullet speed: %.0f blocks/sec", profile.bulletSpeed);
      ImGui::Spacing();
      ImGui::PopID();
    }
    ImGui::EndPopup();
  }
}
}  // namespace craft::app

#include "app/GameplayUi.h"

#include <algorithm>

#include "core/gameplay/Gameplay.h"
#include "imgui.h"
namespace craft::app {
namespace {

void drawBulletRay(ImDrawList& drawList, const ImVec2& from,
                   const ImVec2& to) {
  drawList.AddLine(from, to, IM_COL32(44, 30, 18, 230), 8.0F);
  drawList.AddLine(from, to, IM_COL32(255, 205, 92, 255), 3.5F);
  drawList.AddCircleFilled(to, 4.0F, IM_COL32(255, 246, 208, 255));
}

void drawWeaponIcon(ImDrawList& drawList,
                    const gameplay::UpgradeChoice& choice,
                    const ImVec2& center) {
  switch (choice.weapon) {
    case gameplay::Weapon::Laser: {
      const bool evolved = choice.kind == gameplay::UpgradeKind::Evolution;
      const ImU32 glowColor = evolved ? IM_COL32(255, 35, 48, 90)
                                      : IM_COL32(48, 220, 255, 90);
      const ImU32 beamColor = evolved ? IM_COL32(255, 72, 82, 255)
                                      : IM_COL32(140, 245, 255, 255);
      drawList.AddLine(ImVec2(center.x - 34.0F, center.y),
                       ImVec2(center.x + 34.0F, center.y),
                       glowColor, 10.0F);
      drawList.AddLine(ImVec2(center.x - 32.0F, center.y),
                       ImVec2(center.x + 32.0F, center.y),
                       beamColor, 3.0F);
      break;
    }

    case gameplay::Weapon::Bomb: {
      const bool evolved = choice.kind == gameplay::UpgradeKind::Evolution;
      drawList.AddCircleFilled(center, 17.0F,
                               evolved ? IM_COL32(224, 40, 30, 255)
                                       : IM_COL32(198, 78, 34, 255));
      drawList.AddLine(ImVec2(center.x + 3.0F, center.y - 15.0F),
                       ImVec2(center.x + 15.0F, center.y - 29.0F),
                       evolved ? IM_COL32(255, 92, 28, 255)
                               : IM_COL32(238, 208, 132, 255),
                       3.0F);
      drawList.AddCircleFilled(ImVec2(center.x + 18.0F, center.y - 32.0F),
                               4.0F, evolved ? IM_COL32(255, 52, 28, 255)
                                              : IM_COL32(255, 224, 92, 255));
      break;
    }

    case gameplay::Weapon::Bullet:
      if (choice.kind == gameplay::UpgradeKind::Evolution) {
        const ImVec2 origin(center.x - 25.0F, center.y);
        drawBulletRay(drawList, origin, ImVec2(center.x + 29.0F, center.y));
        drawBulletRay(drawList, origin,
                      ImVec2(center.x + 19.0F, center.y - 25.0F));
        drawBulletRay(drawList, origin,
                      ImVec2(center.x + 19.0F, center.y + 25.0F));
      } else {
        drawBulletRay(drawList, ImVec2(center.x - 29.0F, center.y),
                      ImVec2(center.x + 24.0F, center.y));
      }
      break;

    default:
      break;
  }
}

}  // namespace

bool drawGameplayUi(gameplay::Gameplay& game) {
  const auto& s = game.stats();
  ImGui::SetNextWindowPos(ImVec2(12, 540), ImGuiCond_FirstUseEver);
  ImGui::Begin("Arena status", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("HP %d / 5 | Time %.1f | Kills %d", s.health, s.remainingSeconds,
              s.kills);
  ImGui::Text("Level %d | EXP %d / %d", s.level, s.experience,
              game.experienceToNextLevel());
  int enemyCounts[3] = {};
  for (const auto& enemy : game.enemies())
    ++enemyCounts[static_cast<int>(enemy.kind)];
  ImGui::Text("Enemies: Zombie %d | Golem %d | Spider %d", enemyCounts[0],
              enemyCounts[1], enemyCounts[2]);
  ImGui::TextUnformatted("Arsenal:");
  ImGui::SameLine();
  for (std::size_t i = 0; i < game.weapons().size(); ++i) {
    const auto& state = game.weapons()[i];
    if (i > 0) ImGui::SameLine();
    if (state.evolved) {
      ImGui::Text("%s (Evolved)", gameplay::weaponName(state.weapon));
    } else {
      ImGui::Text("%s %d/%d", gameplay::weaponName(state.weapon),
                  state.upgrades, gameplay::kMaximumWeaponUpgrades);
    }
  }
  if (s.phase == gameplay::Phase::ChoosingUpgrade)
    ImGui::TextUnformatted("LEVEL UP - choose an upgrade in the centre dialog.");
  if (s.phase == gameplay::Phase::Won)
    ImGui::TextUnformatted("Survived! Restart from Game menu.");
  if (s.phase == gameplay::Phase::Lost)
    ImGui::TextUnformatted("Defeated. Restart from Game menu.");
  ImGui::End();

  // The upgrade modal owns presentation only. Gameplay keeps the selection
  // and resume boundary so this layout can evolve independently.
  if (s.phase == gameplay::Phase::Playing) return false;
  const bool upgrading = s.phase == gameplay::Phase::ChoosingUpgrade;
  const char* popup = upgrading ? "LEVEL UP - Choose an upgrade"
                      : s.phase == gameplay::Phase::Won
                          ? "VICTORY - Arena survived"
                          : "DEFEAT - Run ended";
  bool restartRequested = false;
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
    if (!upgrading) {
      ImGui::TextUnformatted(s.phase == gameplay::Phase::Won
                                 ? "VICTORY! You survived the arena."
                                 : "DEFEAT. Your health reached zero.");
      ImGui::TextWrapped(
          "This run has ended. Choose Play again to restart from the arena "
          "spawn.");
      ImGui::Separator();
      ImGui::Text("Kills: %d | Level: %d | Time survived: %.1fs", s.kills,
                  s.level, 180.F - s.remainingSeconds);
      if (ImGui::Button("Play again", ImVec2(-1, 44))) {
        ImGui::CloseCurrentPopup();
        restartRequested = true;
      }
    } else {
      ImGui::Text("Level %d complete!", s.level);
      ImGui::TextWrapped(
          "GAME PAUSED - enemies and the countdown are stopped.");
      ImGui::TextWrapped(
          "Unlock a weapon or improve Bullet, Laser or Bomb. Selection restores "
          "health and resumes the game.");
      ImGui::Separator();
      const auto choices = game.upgradeOptions();
      const float cardGap = 12.0F;
      const float availableWidth = ImGui::GetContentRegionAvail().x;
      const float cardWidth = choices.size() == 1
                                  ? std::min(390.0F, availableWidth)
                                  : std::max(1.0F,
                                             (availableWidth - cardGap) * 0.5F);
      const float cardHeight = std::clamp(
          viewport->WorkSize.y - 160.0F, 230.0F, 300.0F);
      for (std::size_t i = 0; i < choices.size(); ++i) {
        const auto& choice = choices[i];
        ImGui::PushID(static_cast<int>(i));
        if (choices.size() == 1)
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                               (availableWidth - cardWidth) * 0.5F);
        else if (i > 0)
          ImGui::SameLine(0.0F, cardGap);
        ImGui::PushStyleColor(ImGuiCol_ChildBg,
                              ImVec4(0.075F, 0.095F, 0.13F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_Border,
                              ImVec4(0.27F, 0.36F, 0.48F, 1.0F));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0F, 10.0F));
        ImGui::BeginChild("WeaponCard", ImVec2(cardWidth, cardHeight),
                          ImGuiChildFlags_Borders);

        const ImVec2 contentStart = ImGui::GetCursorScreenPos();
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawWeaponIcon(*drawList, choice,
                       ImVec2(contentStart.x + contentWidth * 0.5F,
                              contentStart.y + 34.0F));
        ImGui::Dummy(ImVec2(1.0F, 66.0F));

        const char* name = gameplay::upgradeName(choice);
        const float nameWidth = ImGui::CalcTextSize(name).x;
        ImGui::SetCursorPosX(
            std::max(ImGui::GetCursorPosX(),
                     (ImGui::GetWindowWidth() - nameWidth) * 0.5F));
        ImGui::TextUnformatted(name);
        ImGui::Separator();
        const int profileUpgrades =
            choice.kind == gameplay::UpgradeKind::Evolution
                ? 4
                : choice.kind == gameplay::UpgradeKind::Upgrade
                      ? choice.stage
                      : 0;
        const auto profile =
            gameplay::weaponProfile(choice.weapon, profileUpgrades);
        if (choice.kind == gameplay::UpgradeKind::Unlock)
          ImGui::TextWrapped("%s", gameplay::weaponDescription(choice.weapon));
        ImGui::TextWrapped("%s", gameplay::upgradeDescription(choice));
        ImGui::Spacing();
        ImGui::Text("Damage: %d", profile.baseDamage);
        ImGui::Text("Shot interval: %.2f s", profile.shotCooldown);
        ImGui::Text("Speed: %.0f blocks/s", profile.bulletSpeed);
        if (choice.weapon == gameplay::Weapon::Laser)
          ImGui::Text("Ricochets: %d", profile.maxReflections);
        if (choice.weapon == gameplay::Weapon::Bomb)
          ImGui::Text("Blast radius: %.2f blocks", profile.blastRadius);
        ImGui::SetCursorPosY(std::max(
            ImGui::GetCursorPosY(),
            ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing() -
                ImGui::GetStyle().WindowPadding.y));
        const char* action =
            choice.kind == gameplay::UpgradeKind::Unlock ? "Unlock"
            : choice.kind == gameplay::UpgradeKind::Evolution ? "Evolve"
                                                              : "Upgrade";
        const bool choose = ImGui::Button(action, ImVec2(-1.0F, 36.0F));
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::PopID();
        if (choose && game.chooseUpgrade(i)) {
          ImGui::CloseCurrentPopup();
          break;
        }
      }
    }
    ImGui::EndPopup();
  }
  return restartRequested;
}
}  // namespace craft::app

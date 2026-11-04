#pragma once
namespace craft::gameplay {
class Gameplay;
}
namespace craft::app {
// C: replace this small integration panel with the planned HUD/menu. Gameplay
// exposes Stats, upgradeChoices and chooseUpgrade; UI never owns simulation.
void drawGameplayUi(gameplay::Gameplay& game);
}  // namespace craft::app

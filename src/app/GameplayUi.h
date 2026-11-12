#pragma once
namespace craft::gameplay {
class Gameplay;
}
namespace craft::app {
// Draws the gameplay status and level-up/result dialogs. Gameplay owns stats,
// upgrade choices, selection and the resume boundary; UI stays presentation-only.
// Returns true when the player requests a full scene/gameplay restart.
bool drawGameplayUi(gameplay::Gameplay& game);
}  // namespace craft::app

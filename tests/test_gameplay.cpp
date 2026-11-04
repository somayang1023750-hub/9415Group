#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>

#include "app/FollowCamera.h"
#include "app/PlayerController.h"
#include "core/gameplay/Aim.h"
#include "core/gameplay/Gameplay.h"
#include "world/ArenaWorld.h"

namespace {
using craft::gameplay::Gameplay;
using craft::gameplay::Phase;
craft::world::ArenaWorld emptyWorld() {
  craft::world::ArenaLayout layout;
  layout.obstacles.clear();
  layout.enemySpawnCandidates.clear();
  return craft::world::ArenaWorld(layout);
}
const glm::vec3 player{16, 1.45F, 16};
void killEnemy(Gameplay& game, const craft::world::ArenaWorld& world) {
  REQUIRE(game.spawnEnemy({16, 1.45F, 22}, player, world));
  for (int i = 0; i < 30 && game.stats().kills == 0; ++i)
    game.update(0.1F, player, world, game.enemies().front().position);
  REQUIRE(game.stats().kills == 1);
  REQUIRE(game.orbs().size() == 1);
}
}  // namespace
TEST_CASE("Spawn uses live map clearance, distance, actor occupancy and cap",
          "[gameplay]") {
  craft::world::ArenaWorld world;
  Gameplay game;
  REQUIRE_FALSE(game.spawnEnemy(player, player, world));
  const auto p = world.enemySpawnCandidates().front();
  REQUIRE(game.spawnEnemy(p, player, world));
  REQUIRE_FALSE(game.spawnEnemy(p, player, world));
  auto blocked = world.enemySpawnCandidates().back();
  world.setBlock(static_cast<int>(blocked.x), 1, static_cast<int>(blocked.z),
                 craft::world::BlockType::Obstacle);
  REQUIRE_FALSE(game.spawnEnemy(blocked, player, world));
  game.update(0.1F, player, world);
  for (const auto& e : game.enemies())
    REQUIRE(world.canOccupy(e.position, Gameplay::enemyHalfExtents()));
  auto empty = emptyWorld();
  game.reset();
  int accepted = 0;
  for (int z = 2; z < 12; ++z)
    for (int x = 2; x < 12; ++x)
      accepted += game.spawnEnemy({x + 0.5F, 1.45F, z + 0.5F}, player, empty);
  REQUIRE(accepted == 24);
}
TEST_CASE("Enemies navigate cover without entering solid blocks or the player",
          "[gameplay]") {
  auto layout = craft::world::makeMilestoneOneLayout();
  layout.obstacles = {{15, 18, 3, 2, 2}};
  layout.enemySpawnCandidates.clear();
  craft::world::ArenaWorld world(layout);
  Gameplay game;
  REQUIRE(game.spawnEnemy({16.5F, 1.45F, 23.5F}, player, world));
  for (int i = 0; i < 180; ++i) {
    game.update(0.1F, player, world);
    REQUIRE(world.canOccupy(game.enemies()[0].position,
                            Gameplay::enemyHalfExtents()));
  }
  REQUIRE(glm::length(game.enemies()[0].position - player) < 1.6F);
  REQUIRE(glm::length(game.enemies()[0].position - player) > 0.7F);
}
TEST_CASE(
    "Swept bullets hit enemies, stop at nearer cover and respect cooldown",
    "[gameplay]") {
  auto world = emptyWorld();
  Gameplay game;
  SECTION("Enemy hit and EXP drop") { killEnemy(game, world); }
  SECTION("Cover shields the enemy even within one bullet step") {
    REQUIRE(game.spawnEnemy({16, 1.45F, 22}, player, world));
    world.setBlock(16, 1, 18, craft::world::BlockType::Obstacle);
    for (int i = 0; i < 8; ++i)
      game.update(0.1F, player, world, game.enemies().front().position);
    REQUIRE(game.stats().kills == 0);
    REQUIRE(game.hits().empty());
  }
  SECTION("No shot for zero-length aim; repeated input respects fire rate") {
    game.update(0.01F, player, world, player);
    REQUIRE(game.bullets().empty());
    game.update(0.01F, player, world, glm::vec3(20, 1.45F, 16));
    REQUIRE(game.shots().size() == 1);
    game.update(0.01F, player, world, glm::vec3(20, 1.45F, 16));
    REQUIRE(game.bullets().size() == 1);
    REQUIRE(game.shots().empty());
  }
}
TEST_CASE("EXP magnet, random upgrade choices, pause and restart form a loop",
          "[gameplay]") {
  auto world = emptyWorld();
  Gameplay game;
  for (int kill = 0; kill < 3; ++kill) {
    const int before = game.stats().kills;
    REQUIRE(game.spawnEnemy({16, 1.45F, 22}, player, world));
    for (int i = 0; i < 40 && game.stats().kills == before; ++i)
      game.update(0.1F, player, world, game.enemies().front().position);
    REQUIRE(game.stats().kills == before + 1);
    REQUIRE(game.orbs().size() == 1);
    auto collector = game.orbs()[0].position + glm::vec3(2, 0, 0);
    auto old = game.orbs()[0].position;
    game.update(0.1F, collector, world);
    REQUIRE(glm::length(game.orbs()[0].position - collector) <
            glm::length(old - collector));
    for (int i = 0; i < 10 && !game.orbs().empty(); ++i)
      game.update(0.1F, collector, world);
    REQUIRE(game.orbs().empty());
  }
  REQUIRE(game.stats().phase == Phase::ChoosingUpgrade);
  REQUIRE(game.upgradeChoices().size() == 2);
  REQUIRE(game.upgradeChoices()[0] != game.upgradeChoices()[1]);
  const float time = game.stats().remainingSeconds;
  game.update(0.1F, player, world);
  REQUIRE(game.stats().remainingSeconds == time);
  REQUIRE_FALSE(game.chooseUpgrade(2));
  auto weapon = game.upgradeChoices()[0];
  REQUIRE(game.chooseUpgrade(0));
  REQUIRE(game.stats().level == 2);
  REQUIRE(game.stats().experience == 0);
  REQUIRE(game.stats().weapon == weapon);
  REQUIRE(game.stats().phase == Phase::Playing);
  game.reset();
  REQUIRE(game.stats().level == 1);
  REQUIRE(game.stats().health == 5);
  REQUIRE(game.enemies().empty());
  REQUIRE(game.bullets().empty());
  REQUIRE(game.stats().remainingSeconds == 180);
}
TEST_CASE(
    "Contact damage has invulnerability and terminal states stop simulation",
    "[gameplay]") {
  auto world = emptyWorld();
  Gameplay game;
  SECTION("Survival timer wins") {
    for (int i = 0; i < 1900; ++i) game.update(0.1F, player, world);
    REQUIRE(game.stats().phase == Phase::Won);
  }
  SECTION("Contact eventually loses") {
    REQUIRE(game.spawnEnemy({16, 1.45F, 21}, player, world));
    for (int i = 0; i < 200 && game.stats().health == 5; ++i)
      game.update(0.1F, player, world);
    REQUIRE(game.stats().health == 4);
    game.update(0.1F, player, world);
    REQUIRE(game.stats().health == 4);
    for (int i = 0; i < 60; ++i) game.update(0.1F, player, world);
    REQUIRE(game.stats().phase == Phase::Lost);
    float time = game.stats().remainingSeconds;
    game.update(0.1F, player, world);
    REQUIRE(game.stats().remainingSeconds == time);
  }
}
TEST_CASE(
    "Existing movement normalizes diagonals, cancels opposites and blocks "
    "walls",
    "[movement]") {
  auto world = emptyWorld();
  craft::app::PlayerController straight, diagonal;
  REQUIRE(straight.setPosition(player, world));
  REQUIRE(diagonal.setPosition(player, world));
  straight.update({true, false, false, false}, 0.1F, {0, 0, 1}, {1, 0, 0},
                  world);
  diagonal.update({true, false, false, true}, 0.1F, {0, 0, 1}, {1, 0, 0},
                  world);
  REQUIRE(glm::length(straight.position() - player) ==
          Catch::Approx(glm::length(diagonal.position() - player)));
  auto before = diagonal.position();
  diagonal.update({true, true, true, true}, 0.1F, {0, 0, 1}, {1, 0, 0}, world);
  REQUIRE(diagonal.position() == before);
  REQUIRE(straight.setPosition({1.5F, 1.45F, 16}, world));
  for (int i = 0; i < 10; ++i)
    straight.update({false, false, true, false}, 0.1F, {0, 0, 1}, {1, 0, 0},
                    world);
  REQUIRE(world.canOccupy(straight.position(), {0.42F, 0.44F, 0.42F}));
}
TEST_CASE("Camera aiming maps logical window pixels onto the actor plane",
          "[aim]") {
  craft::app::FollowCamera camera;
  const auto vp = camera.viewProjection(player, 2560, 1440);
  auto aim = craft::gameplay::aimOnPlane(vp, 640, 360, 1280, 720);
  REQUIRE(aim.has_value());
  REQUIRE(glm::length(*aim - player) < 0.001F);
  const glm::vec3 target(20, 1.45F, 19);
  auto clip = vp * glm::vec4(target, 1);
  clip /= clip.w;
  aim = craft::gameplay::aimOnPlane(vp, (clip.x + 1) * 640, (1 - clip.y) * 360,
                                    1280, 720);
  REQUIRE(aim.has_value());
  REQUIRE(glm::length(*aim - target) < 0.001F);
  REQUIRE_FALSE(craft::gameplay::aimOnPlane(vp, 0, 0, 0, 0));
}

TEST_CASE("Weapon profiles have distinct cadence, projectile speed and damage",
          "[weapons]") {
  using craft::gameplay::Weapon;
  using craft::gameplay::weaponProfile;
  const auto heavy = weaponProfile(Weapon::Heavy);
  const auto balanced = weaponProfile(Weapon::Balanced);
  const auto rapid = weaponProfile(Weapon::Rapid);
  REQUIRE(heavy.shotCooldown > balanced.shotCooldown * 2);
  REQUIRE(balanced.shotCooldown > rapid.shotCooldown * 2);
  REQUIRE(rapid.bulletSpeed > balanced.bulletSpeed);
  REQUIRE(balanced.bulletSpeed == heavy.bulletSpeed * 2);
  REQUIRE(heavy.baseDamage > balanced.baseDamage);
  REQUIRE(balanced.baseDamage > rapid.baseDamage);
  auto world = emptyWorld();
  // Exercise every profile through the actual upgrade and firing boundary.
  for (auto desired : {Weapon::Balanced, Weapon::Rapid, Weapon::Heavy}) {
    bool verified = false;
    for (unsigned seed = 0; seed < 20 && !verified; ++seed) {
      Gameplay game(seed);
      for (int kill = 0; kill < 3; ++kill) {
        const int before = game.stats().kills;
        REQUIRE(game.spawnEnemy({16, 1.45F, 22}, player, world));
        for (int i = 0; i < 40 && game.stats().kills == before; ++i)
          game.update(.1F, player, world, game.enemies().front().position);
        REQUIRE(game.orbs().size() == 1);
        game.update(.1F, game.orbs().front().position, world);
      }
      const auto choices = game.upgradeChoices();
      for (std::size_t i = 0; i < choices.size(); ++i) {
        if (choices[i] != desired) continue;
        REQUIRE(game.chooseUpgrade(i));
        for (int frame = 0; frame < 25; ++frame)
          game.update(.1F, player, world);
        game.update(.01F, player, world, glm::vec3(20, 1.45F, 16));
        REQUIRE(game.shots().size() == 1);
        REQUIRE(game.bullets().size() == 1);
        const auto profile = weaponProfile(desired);
        REQUIRE(glm::length(game.bullets()[0].velocity) ==
                Catch::Approx(profile.bulletSpeed));
        REQUIRE(game.bullets()[0].damage == profile.baseDamage + 1);
        REQUIRE(game.bullets()[0].weapon == desired);
        game.update(.01F, player, world, glm::vec3(20, 1.45F, 16));
        REQUIRE(game.shots().empty());
        verified = true;
        break;
      }
    }
    REQUIRE(verified);
  }
}

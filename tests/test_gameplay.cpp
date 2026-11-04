#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>
#include <limits>

#include "app/FollowCamera.h"
#include "app/PlayerController.h"
#include "core/gameplay/Aim.h"
#include "core/gameplay/Gameplay.h"
#include "world/ArenaWorld.h"

namespace {
using craft::gameplay::Gameplay;
using craft::gameplay::kActorCentreY;
using craft::gameplay::Phase;
craft::world::ArenaWorld emptyWorld() {
  craft::world::ArenaLayout layout;
  layout.obstacles.clear();
  layout.enemySpawnCandidates.clear();
  return craft::world::ArenaWorld(layout);
}
const glm::vec3 player{16, kActorCentreY, 16};
void killEnemy(Gameplay& game, const craft::world::ArenaWorld& world) {
  REQUIRE(game.spawnEnemy({16, kActorCentreY, 22}, player, world));
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
      accepted +=
          game.spawnEnemy({x + 0.5F, kActorCentreY, z + 0.5F}, player, empty);
  REQUIRE(accepted == 24);
}
TEST_CASE("Enemies navigate cover without entering solid blocks or the player",
          "[gameplay]") {
  auto layout = craft::world::makeMilestoneOneLayout();
  layout.obstacles = {{15, 18, 3, 2, 2}};
  layout.enemySpawnCandidates.clear();
  craft::world::ArenaWorld world(layout);
  Gameplay game;
  REQUIRE(game.spawnEnemy({16.5F, kActorCentreY, 23.5F}, player, world));
  for (int i = 0; i < 180; ++i) {
    game.update(0.1F, player, world);
    REQUIRE(world.canOccupy(game.enemies()[0].position,
                            Gameplay::enemyHalfExtents()));
  }
  REQUIRE(glm::length(game.enemies()[0].position - player) < 1.6F);
  REQUIRE(glm::length(game.enemies()[0].position - player) >
          craft::gameplay::playerEnemySeparation() - 0.02F);
}
TEST_CASE(
    "Swept bullets hit enemies, stop at nearer cover and respect cooldown",
    "[gameplay]") {
  auto world = emptyWorld();
  Gameplay game;
  SECTION("Enemy hit and EXP drop") { killEnemy(game, world); }
  SECTION("Cover shields the enemy even within one bullet step") {
    REQUIRE(game.spawnEnemy({16, kActorCentreY, 22}, player, world));
    world.setBlock(16, 1, 18, craft::world::BlockType::Obstacle);
    for (int i = 0; i < 8; ++i)
      game.update(0.1F, player, world, game.enemies().front().position);
    REQUIRE(game.stats().kills == 0);
    REQUIRE(game.hits().empty());
  }
  SECTION("No shot for zero-length aim; repeated input respects fire rate") {
    game.update(0.01F, player, world, player);
    REQUIRE(game.bullets().empty());
    game.update(0.01F, player, world, glm::vec3(20, kActorCentreY, 16));
    REQUIRE(game.shots().size() == 1);
    game.update(0.01F, player, world, glm::vec3(20, kActorCentreY, 16));
    REQUIRE(game.bullets().size() == 1);
    REQUIRE(game.shots().empty());
  }
}
TEST_CASE("EXP magnet, weapon choices, pause and restart form a loop",
          "[gameplay]") {
  auto world = emptyWorld();
  Gameplay game;
  for (int kill = 0; kill < 3; ++kill) {
    const int before = game.stats().kills;
    REQUIRE(game.spawnEnemy({16, kActorCentreY, 22}, player, world));
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
    REQUIRE(game.spawnEnemy({16, kActorCentreY, 21}, player, world));
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
  REQUIRE(straight.setPosition({1.5F, kActorCentreY, 16}, world));
  for (int i = 0; i < 10; ++i)
    straight.update({false, false, true, false}, 0.1F, {0, 0, 1}, {1, 0, 0},
                    world);
  REQUIRE(world.canOccupy(straight.position(),
                          craft::gameplay::playerHalfExtents()));
}
TEST_CASE("Camera aiming maps logical window pixels onto the actor plane",
          "[aim]") {
  craft::app::FollowCamera camera;
  const auto vp = camera.viewProjection(player, 2560, 1440);
  auto aim = craft::gameplay::aimOnPlane(vp, 640, 360, 1280, 720);
  REQUIRE(aim.has_value());
  REQUIRE(glm::length(*aim - player) < 0.001F);
  const glm::vec3 target(20, kActorCentreY, 19);
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
  const auto heavy = weaponProfile(Weapon::Bomb);
  const auto balanced = weaponProfile(Weapon::Balance);
  const auto rapid = weaponProfile(Weapon::Laser);
  REQUIRE(heavy.shotCooldown > balanced.shotCooldown * 2);
  REQUIRE(balanced.shotCooldown > rapid.shotCooldown * 2);
  REQUIRE(rapid.bulletSpeed > balanced.bulletSpeed);
  REQUIRE(balanced.bulletSpeed > heavy.bulletSpeed * 2);
  REQUIRE(heavy.baseDamage > balanced.baseDamage);
  REQUIRE(balanced.baseDamage > rapid.baseDamage);
  auto world = emptyWorld();
  // Exercise every profile through the actual upgrade and firing boundary.
  for (auto desired : {Weapon::Balance, Weapon::Laser, Weapon::Bomb}) {
    bool verified = false;
    for (unsigned seed = 0; seed < 20 && !verified; ++seed) {
      Gameplay game(seed);
      for (int kill = 0; kill < 3; ++kill) {
        const int before = game.stats().kills;
        REQUIRE(game.spawnEnemy({16, kActorCentreY, 22}, player, world));
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
        game.update(.01F, player, world, glm::vec3(20, kActorCentreY, 16));
        REQUIRE(game.shots().size() == 1);
        REQUIRE(game.bullets().size() == 1);
        const auto profile = weaponProfile(desired);
        REQUIRE(glm::length(game.bullets()[0].velocity) ==
                Catch::Approx(profile.bulletSpeed));
        REQUIRE(game.bullets()[0].damage == profile.baseDamage);
        REQUIRE(game.bullets()[0].weapon == desired);
        game.update(.01F, player, world, glm::vec3(20, kActorCentreY, 16));
        REQUIRE(game.shots().empty());
        verified = true;
        break;
      }
    }
    REQUIRE(verified);
  }
}

namespace {
using craft::gameplay::Weapon;
Gameplay equip(Weapon desired) {
  const auto world = emptyWorld();
  for (unsigned seed = 0; seed < 30; ++seed) {
    Gameplay game(seed);
    for (int kill = 0; kill < 3; ++kill) {
      const int before = game.stats().kills;
      REQUIRE(game.spawnEnemy({16, kActorCentreY, 22}, player, world));
      for (int i = 0; i < 40 && game.stats().kills == before; ++i)
        game.update(.1F, player, world, game.enemies().front().position);
      REQUIRE(game.orbs().size() == 1);
      const auto collector = game.orbs()[0].position;
      game.update(.1F, collector, world);
    }
    const auto choices = game.upgradeChoices();
    for (std::size_t i = 0; i < choices.size(); ++i)
      if (choices[i] == desired) {
        REQUIRE(game.chooseUpgrade(i));
        for (int frame = 0; frame < 35; ++frame)
          game.update(.1F, player, world);
        return game;
      }
  }
  FAIL("Requested weapon was never offered");
  return Gameplay{};
}
}  // namespace
TEST_CASE("Laser pierces enemies once each and reflects off exactly one wall",
          "[weapons][laser]") {
  auto world = emptyWorld();
  auto game = equip(Weapon::Laser);
  const glm::vec3 shooter(4.5F, kActorCentreY, 16.5F);
  SECTION(
      "One shot damages several enemies, not repeatedly while inside them") {
    // Narrow lane keeps the moving targets in the beam while allowing the
    // laser to pass through all of them; open-space AI deliberately sidesteps
    // queues.
    for (int x = 6; x <= 23; ++x)
      for (int z : {15, 17})
        world.setBlock(x, 1, z, craft::world::BlockType::Obstacle);
    for (float x : {10.5F, 14.5F, 18.5F})
      REQUIRE(game.spawnEnemy({x, kActorCentreY, 16.5F}, shooter, world));
    game.update(.01F, shooter, world, glm::vec3(27, kActorCentreY, 16.5F));
    for (int i = 0; i < 60; ++i) game.update(.01F, shooter, world);
    REQUIRE(game.enemies().size() == 3);
    for (const auto& enemy : game.enemies()) REQUIRE(enemy.health == 3);
    for (int i = 0; i < 35; ++i) game.update(.01F, shooter, world);
    for (const auto& enemy : game.enemies()) REQUIRE(enemy.health == 3);
  }
  SECTION("Reflection preserves speed and stops on the second obstacle") {
    world.setBlock(20, 1, 16, craft::world::BlockType::Obstacle);
    world.setBlock(12, 1, 16, craft::world::BlockType::Obstacle);
    const glm::vec3 origin(16.5F, kActorCentreY, 16.5F);
    game.update(.1F, origin, world, glm::vec3(25, kActorCentreY, 16.5F));
    REQUIRE(game.bullets().size() == 1);
    REQUIRE(game.bullets()[0].reflections == 1);
    REQUIRE(game.bullets()[0].velocity.x == Catch::Approx(-40));
    REQUIRE(game.bullets()[0].position.x < 19.9F);
    for (int i = 0; i < 3; ++i) game.update(.1F, origin, world);
    REQUIRE(game.bullets().empty());
  }
}
TEST_CASE("Bombs explode on enemy or cover with full radial damage",
          "[weapons][bomb]") {
  auto world = emptyWorld();
  auto game = equip(Weapon::Bomb);
  const glm::vec3 shooter(4.5F, kActorCentreY, 16.5F);
  SECTION("Cover triggers an explosion affecting enemies behind it") {
    world.setBlock(10, 1, 16, craft::world::BlockType::Obstacle);
  }
  SECTION("An enemy directly triggers the explosion") {}
  REQUIRE(game.spawnEnemy({11.5F, kActorCentreY, 16.5F}, shooter, world));
  REQUIRE(game.spawnEnemy({11.5F, kActorCentreY, 18.5F}, shooter, world));
  REQUIRE(game.spawnEnemy({24.5F, kActorCentreY, 25.5F}, shooter, world));
  const int baseline = game.stats().kills;
  game.update(.01F, shooter, world, glm::vec3(26, kActorCentreY, 16.5F));
  for (int i = 0; i < 150 && game.explosions().empty(); ++i)
    game.update(.01F, shooter, world);
  REQUIRE(game.explosions().size() == 1);
  REQUIRE(game.explosions()[0].radius == Gameplay::blastRadius());
  REQUIRE(game.stats().kills == baseline + 2);
  REQUIRE(game.enemies().size() == 1);
  REQUIRE(game.enemies()[0].health == 4);
  REQUIRE(game.bullets().empty());
  REQUIRE(game.orbs().size() == 2);
  for (int i = 0; i < 45; ++i) game.update(.01F, shooter, world);
  REQUIRE(game.explosions().empty());
  REQUIRE(game.stats().kills == baseline + 2);
}
TEST_CASE(
    "Weapon hit counts stay two four and one after switching and leveling",
    "[weapons]") {
  for (auto weapon : {Weapon::Balance, Weapon::Laser, Weapon::Bomb}) {
    auto game = equip(weapon);
    const auto world = emptyWorld();
    const glm::vec3 shooter(4.5F, kActorCentreY, 16.5F);
    REQUIRE(game.spawnEnemy({14.5F, kActorCentreY, 16.5F}, shooter, world));
    const int expected = weapon == Weapon::Balance ? 2
                         : weapon == Weapon::Laser ? 4
                                                   : 1;
    const int baseline = game.stats().kills;
    int fired = 0;
    for (int frame = 0; frame < 250 && game.stats().kills == baseline;
         ++frame) {
      std::optional<glm::vec3> aim;
      bool ready = game.bullets().empty();
      if (weapon == Weapon::Laser && !game.bullets().empty())
        ready = !game.bullets().back().hitEnemies.empty();
      if (ready && !game.enemies().empty()) aim = game.enemies()[0].position;
      game.update(.01F, shooter, world, aim);
      fired += static_cast<int>(game.shots().size());
    }
    REQUIRE(game.stats().kills == baseline + 1);
    REQUIRE(fired == expected);
  }
}
TEST_CASE("Enemy waypoints stay stable through turns and crowds reroute",
          "[gameplay][navigation]") {
  auto layout = craft::world::makeMilestoneOneLayout();
  layout.obstacles = {{13, 18, 6, 1, 2}};
  layout.enemySpawnCandidates.clear();
  craft::world::ArenaWorld world(layout);
  Gameplay game;
  for (float x : {14.5F, 15.5F, 16.5F, 17.5F})
    REQUIRE(game.spawnEnemy({x, kActorCentreY, 23.5F}, player, world));
  for (int frame = 0; frame < 50; ++frame) {
    game.update(.1F, player, world);
    for (const auto& e : game.enemies()) {
      REQUIRE(world.canOccupy(e.position, Gameplay::enemyHalfExtents()));
      for (const auto& other : game.enemies())
        if (e.id != other.id)
          REQUIRE((std::abs(e.position.x - other.position.x) >=
                       craft::gameplay::enemySeparation() - .001F ||
                   std::abs(e.position.z - other.position.z) >=
                       craft::gameplay::enemySeparation() - .001F));
    }
  }
  int arrived = 0;
  for (const auto& e : game.enemies())
    if (e.position.z < 18) ++arrived;
  REQUIRE(arrived >= 3);
  const auto old = game.enemies();
  const glm::vec3 movedPlayer(26.5F, kActorCentreY, 16.5F);
  for (int frame = 0; frame < 30; ++frame) game.update(.1F, movedPlayer, world);
  int progressed = 0;
  for (std::size_t i = 0; i < old.size(); ++i)
    if (glm::length(game.enemies()[i].position - old[i].position) > 3)
      ++progressed;
  REQUIRE(progressed >= 3);
}
TEST_CASE("Enemy speed is slightly below the default player speed",
          "[gameplay]") {
  const auto world = emptyWorld();
  Gameplay game;
  const glm::vec3 origin(16.5F, kActorCentreY, 24.5F),
      target(16.5F, kActorCentreY, 16.5F);
  REQUIRE(game.spawnEnemy(origin, target, world));
  game.update(.1F, target, world);
  REQUIRE(glm::length(game.enemies()[0].position - origin) ==
          Catch::Approx(.43F).margin(.001));
  craft::app::PlayerController controller;
  REQUIRE(Gameplay::enemySpeed() < controller.moveSpeed());
  REQUIRE(Gameplay::enemySpeed() > controller.moveSpeed() * .8F);
}

TEST_CASE("Accepted level-up fully heals while invalid choices do not",
          "[progression]") {
  const auto world = emptyWorld();
  Gameplay game;
  REQUIRE(game.spawnEnemy({16, kActorCentreY, 21}, player, world));
  for (int frame = 0; frame < 200 && game.stats().health == 5; ++frame)
    game.update(.01F, player, world);
  REQUIRE(game.stats().health == 4);
  for (int kill = 0; kill < 3; ++kill) {
    const int before = game.stats().kills;
    if (game.enemies().empty())
      REQUIRE(game.spawnEnemy({16, kActorCentreY, 22}, player, world));
    for (int frame = 0; frame < 150 && game.stats().kills == before; ++frame)
      game.update(.01F, player, world, game.enemies().front().position);
    REQUIRE(game.stats().kills == before + 1);
    if (!game.orbs().empty()) {
      const auto collector = game.orbs()[0].position;
      game.update(.01F, collector, world);
    }
  }
  REQUIRE(game.stats().phase == Phase::ChoosingUpgrade);
  REQUIRE(game.stats().health < 5);
  const auto health = game.stats().health;
  REQUIRE_FALSE(game.chooseUpgrade(99));
  REQUIRE(game.stats().health == health);
  REQUIRE(game.chooseUpgrade(0));
  REQUIRE(game.stats().health == 5);
  REQUIRE(game.stats().phase == Phase::Playing);
}
TEST_CASE("Local overhead camera follows without fitting the whole arena",
          "[camera]") {
  craft::app::FollowCamera camera;
  const glm::vec3 start(16, kActorCentreY, 16), moved(22, kActorCentreY, 9);
  const auto offset = camera.worldPosition(start) - start;
  const float horizontal = glm::length(glm::vec3(offset.x, 0, offset.z));
  REQUIRE(horizontal / offset.y == Catch::Approx(7.F / 26));
  REQUIRE(glm::length((camera.worldPosition(moved) - moved) - offset) < .001F);
  for (const auto size :
       {glm::ivec2(1280, 720), glm::ivec2(720, 1280), glm::ivec2(2560, 1080)}) {
    const auto vp = camera.viewProjection(start, size.x, size.y);
    auto centre = vp * glm::vec4(start, 1);
    centre /= centre.w;
    REQUIRE(std::abs(centre.x) < .0001F);
    REQUIRE(std::abs(centre.y) < .0001F);
    auto follow = camera.viewProjection(moved, size.x, size.y);
    const auto local = start + glm::vec3(2, 0, 1);
    const auto movedLocal = moved + glm::vec3(2, 0, 1);
    REQUIRE(glm::length(vp * glm::vec4(local, 1) -
                        follow * glm::vec4(movedLocal, 1)) < .001F);
    const auto near = vp * glm::vec4(start + glm::vec3(2, 0, 2), 1);
    const auto far = vp * glm::vec4(start + glm::vec3(14, 0, 14), 1);
    REQUIRE(std::abs(near.x / near.w) < 1);
    REQUIRE(std::abs(near.y / near.w) < 1);
    REQUIRE(std::abs(far.y / far.w) > 1);
    const auto aim = craft::gameplay::aimOnPlane(follow, size.x / 2.,
                                                 size.y / 2., size.x, size.y);
    REQUIRE(aim.has_value());
    REQUIRE(glm::length(*aim - moved) < .001F);
  }
  REQUIRE_THROWS(camera.viewProjection(start, 0, 720));
}
TEST_CASE("Smaller actors use shared grounded render and collision proportions",
          "[actors]") {
  const auto world = emptyWorld();
  craft::app::PlayerController controller;
  REQUIRE(controller.setPosition(world.playerSpawn(), world));
  REQUIRE(controller.position().y == kActorCentreY);
  REQUIRE(craft::gameplay::kActorScale == Catch::Approx(.7));
  REQUIRE(craft::gameplay::playerHalfExtents().x == Catch::Approx(.42F * .7F));
  REQUIRE(Gameplay::enemyHalfExtents().x == Catch::Approx(.35F * .7F));
  REQUIRE(kActorCentreY - Gameplay::enemyHalfExtents().y >= 1);
  Gameplay game;
  REQUIRE(game.spawnEnemy({16, 1.45F, 23}, controller.position(), world));
  REQUIRE(game.enemies()[0].position.y == kActorCentreY);
  const float invalid = std::numeric_limits<float>::quiet_NaN();
  REQUIRE_FALSE(controller.setPosition({16, invalid, 16}, world));
  REQUIRE_FALSE(
      game.spawnEnemy({24, invalid, 24}, controller.position(), world));
  // Clear for the smaller body, but the previous body would intersect the wall.
  REQUIRE(controller.setPosition({1.31F, 1.45F, 16}, world));
  REQUIRE(world.canOccupy(controller.position(),
                          craft::gameplay::playerHalfExtents()));
  REQUIRE_FALSE(world.canOccupy(controller.position(), {.42F, .44F, .42F}));
}

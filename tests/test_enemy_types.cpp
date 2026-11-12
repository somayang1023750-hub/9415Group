#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <glm/geometric.hpp>

#include "core/gameplay/Gameplay.h"
#include "world/ArenaWorld.h"

namespace {
using namespace craft::gameplay;
using craft::world::ArenaLevel;
using craft::world::ArenaWorld;
using craft::world::BlockType;
const glm::vec3 player(16, kActorCentreY, 16);
ArenaWorld emptyArena() {
  craft::world::ArenaLayout l;
  l.obstacles.clear();
  l.enemySpawnCandidates.clear();
  return ArenaWorld(l);
}
bool overlap(const Enemy& a, const Enemy& b) {
  const auto h =
      enemyProfile(a.kind).halfExtents + enemyProfile(b.kind).halfExtents;
  return std::abs(a.position.x - b.position.x) < h.x - .0001F &&
         std::abs(a.position.z - b.position.z) < h.z - .0001F;
}
}  // namespace
TEST_CASE(
    "Enemy profiles retain exact requested health damage and reward ratios",
    "[enemies]") {
  const auto z = enemyProfile(EnemyKind::Zombie),
             g = enemyProfile(EnemyKind::Golem),
             s = enemyProfile(EnemyKind::Spider);
  REQUIRE(g.health == z.health * 2);
  REQUIRE(g.contactDamage == z.contactDamage * 2);
  REQUIRE(g.experience == z.experience * 2);
  REQUIRE(s.health == Catch::Approx(z.health * .85F));
  REQUIRE(s.health == Catch::Approx(3.4F));
  REQUIRE(s.contactDamage == z.contactDamage);
  REQUIRE(s.experience == z.experience);
  REQUIRE(g.speed < z.speed);
  REQUIRE(s.speed > z.speed * 1.25F);
  REQUIRE(s.speed < 5);
  REQUIRE(g.halfExtents.x > z.halfExtents.x);
  REQUIRE(s.halfExtents.y < z.halfExtents.y);
  for (auto kind : {EnemyKind::Zombie, EnemyKind::Golem, EnemyKind::Spider}) {
    const auto p = enemyProfile(kind);
    REQUIRE(p.centreY - p.halfExtents.y >= 1);
    REQUIRE(p.centreY - p.halfExtents.y <= 1.02F);
  }
}
TEST_CASE("Day and Night spawn weights ramp only after their release times",
          "[enemies]") {
  for (int sec = 0; sec <= 180; ++sec) {
    const auto d = enemySpawnWeights(ArenaLevel::Day, static_cast<float>(sec));
    const auto n =
        enemySpawnWeights(ArenaLevel::Night, static_cast<float>(sec));
    REQUIRE(d[2] == 0);
    REQUIRE(d[0] + d[1] + d[2] == Catch::Approx(1));
    REQUIRE(n[0] + n[1] + n[2] == Catch::Approx(1));
    for (float w : d) REQUIRE(w >= 0);
    for (float w : n) REQUIRE(w >= 0);
    if (sec <= 60) REQUIRE(d[1] == 0);
    if (sec <= 30) REQUIRE(n[1] == 0);
    if (sec <= 60) REQUIRE(n[2] == 0);
  }
  auto d = enemySpawnWeights(ArenaLevel::Day, 90);
  REQUIRE(d[1] == Catch::Approx(.15));
  d = enemySpawnWeights(ArenaLevel::Day, 120);
  REQUIRE(d[0] == Catch::Approx(.7));
  REQUIRE(d[1] == Catch::Approx(.3));
  auto n = enemySpawnWeights(ArenaLevel::Night, 60);
  REQUIRE(n[1] == Catch::Approx(.075));
  n = enemySpawnWeights(ArenaLevel::Night, 120);
  REQUIRE(n[0] == Catch::Approx(.6));
  REQUIRE(n[1] == Catch::Approx(.15));
  REQUIRE(n[2] == Catch::Approx(.25));
}
TEST_CASE("Autonomous mixed batches follow level weights and respect live cap",
          "[enemies]") {
  const auto empty = emptyArena();
  auto layout = craft::world::ArenaLayout{};
  layout.enemySpawnCandidates = {{3.5F, 1.45F, 3.5F},   {16.5F, 1.45F, 3.5F},
                                 {28.5F, 1.45F, 3.5F},  {3.5F, 1.45F, 16.5F},
                                 {28.5F, 1.45F, 16.5F}, {3.5F, 1.45F, 28.5F},
                                 {16.5F, 1.45F, 28.5F}, {28.5F, 1.45F, 28.5F}};
  const ArenaWorld spawnMap(layout);
  for (auto level : {ArenaLevel::Day, ArenaLevel::Night}) {
    int counts[3] = {};
    int total = 0;
    for (unsigned seed = 0; seed < 100; ++seed) {
      Gameplay game(seed);
      game.reset(level);
      for (int frame = 0; frame < 1250; ++frame)
        game.update(.1F, player, empty);
      for (int frame = 0; frame < 21 && game.enemies().empty(); ++frame)
        game.update(.1F, player, spawnMap);
      REQUIRE(game.enemies().size() >= 5);
      REQUIRE(game.enemies().size() <= 24);
      for (const auto& e : game.enemies()) {
        ++counts[static_cast<int>(e.kind)];
        ++total;
      }
    }
    const auto expected = enemySpawnWeights(level, 125);
    for (int kind = 0; kind < 3; ++kind) {
      INFO("level=" << static_cast<int>(level) << " kind=" << kind);
      REQUIRE(static_cast<float>(counts[kind]) / total ==
              Catch::Approx(expected[kind]).margin(.045F));
    }
  }
}
TEST_CASE("Typed spawns ground bodies and reject insufficient clearance",
          "[enemies]") {
  const auto world = emptyArena();
  for (auto kind : {EnemyKind::Zombie, EnemyKind::Golem, EnemyKind::Spider}) {
    Gameplay game;
    const bool accepted =
        game.spawnEnemy({1.30F, 1.45F, 8}, player, world, kind);
    REQUIRE(accepted == (kind != EnemyKind::Golem));
    game.reset();
    REQUIRE(game.spawnEnemy({16, 1.45F, 23}, player, world, kind));
    const auto& e = game.enemies()[0];
    const auto p = enemyProfile(kind);
    REQUIRE(e.kind == kind);
    REQUIRE(e.health == p.health);
    REQUIRE(e.position.y == p.centreY);
    REQUIRE(world.canOccupy(e.position, p.halfExtents));
    REQUIRE_FALSE(game.spawnEnemy(e.position, player, world, EnemyKind::Golem));
  }
}
TEST_CASE("Mixed enemy contact deals typed damage with player invulnerability",
          "[enemies]") {
  const auto world = emptyArena();
  for (auto kind : {EnemyKind::Zombie, EnemyKind::Golem, EnemyKind::Spider}) {
    Gameplay game;
    REQUIRE(game.spawnEnemy({16, 1.45F, 21}, player, world, kind));
    for (int frame = 0; frame < 400 && game.stats().health == 5; ++frame)
      game.update(.01F, player, world);
    REQUIRE(game.stats().health == 5 - enemyProfile(kind).contactDamage);
    REQUIRE(game.attacks().size() == 1);
    REQUIRE(game.attacks()[0].enemyId == game.enemies()[0].id);
    game.update(.1F, player, world);
    REQUIRE(game.stats().health == 5 - enemyProfile(kind).contactDamage);
  }
}
TEST_CASE(
    "Typed projectile hits preserve fractional HP and drop typed experience "
    "once",
    "[enemies]") {
  const auto world = emptyArena();
  for (auto kind : {EnemyKind::Zombie, EnemyKind::Golem, EnemyKind::Spider}) {
    Gameplay game;
    REQUIRE(game.spawnEnemy({16, 1.45F, 22}, player, world, kind));
    bool checkedFirstHit = false;
    for (int frame = 0; frame < 400 && game.stats().kills == 0; ++frame) {
      REQUIRE_FALSE(game.enemies().empty());
      game.update(.01F, player, world, game.enemies()[0].position);
      if (!checkedFirstHit && !game.hits().empty()) {
        REQUIRE_FALSE(game.enemies().empty());
        REQUIRE(game.enemies()[0].health ==
                Catch::Approx(enemyProfile(kind).health - 2));
        checkedFirstHit = true;
      }
    }
    REQUIRE(checkedFirstHit);
    REQUIRE(game.stats().kills == 1);
    REQUIRE(game.orbs().size() == 1);
    REQUIRE(game.orbs()[0].value == enemyProfile(kind).experience);
    const auto collector = game.orbs()[0].position;
    game.update(.01F, collector, world);
    REQUIRE(game.stats().experience == enemyProfile(kind).experience);
    game.update(.01F, collector, world);
    REQUIRE(game.stats().kills == 1);
  }
}
TEST_CASE("Mixed bodies reroute around cover without crossing actors or walls",
          "[enemies]") {
  auto layout = craft::world::ArenaLayout{};
  layout.obstacles = {{13, 18, 9, 1, 2}};
  ArenaWorld world(layout);
  Gameplay game;
  for (int i = 0; i < 6; ++i)
    REQUIRE(game.spawnEnemy({12.5F + i * 2, 1.45F, 23.5F}, player, world,
                            static_cast<EnemyKind>(i % 3)));
  const auto original = game.enemies();
  for (int frame = 0; frame < 120; ++frame) {
    const auto before = game.enemies();
    const glm::vec3 target =
        frame < 60 ? player : glm::vec3(26.5F, kActorCentreY, 16.5F);
    game.update(.05F, target, world);
    REQUIRE(game.stats().phase == Phase::Playing);
    for (std::size_t i = 0; i < game.enemies().size(); ++i) {
      const auto& e = game.enemies()[i];
      const auto p = enemyProfile(e.kind);
      REQUIRE(world.canOccupy(e.position, p.halfExtents));
      REQUIRE(glm::length(e.position - before[i].position) <=
              p.speed * .05F + .001F);
      for (const auto& other : game.enemies())
        if (e.id != other.id) REQUIRE_FALSE(overlap(e, other));
    }
  }
  int progressed = 0;
  for (std::size_t i = 0; i < original.size(); ++i)
    if (glm::length(game.enemies()[i].position - original[i].position) > 3)
      ++progressed;
  REQUIRE(progressed >= 5);
}
TEST_CASE(
    "Golem can approach a player next to a boundary despite its larger body",
    "[enemies]") {
  const auto world = emptyArena();
  Gameplay game;
  const glm::vec3 target(1.31F, kActorCentreY, 16.5F);
  REQUIRE(
      game.spawnEnemy({6.5F, 1.45F, 16.5F}, target, world, EnemyKind::Golem));
  for (int frame = 0; frame < 300 && game.stats().health == 5; ++frame)
    game.update(.01F, target, world);
  REQUIRE(game.stats().health == 3);
  REQUIRE(world.canOccupy(game.enemies()[0].position,
                          enemyProfile(EnemyKind::Golem).halfExtents));
}
TEST_CASE("Level reset preserves selected encounter rules and resets ramp time",
          "[enemies]") {
  const auto world = emptyArena();
  Gameplay game;
  game.reset(ArenaLevel::Night);
  for (int i = 0; i < 400; ++i) game.update(.1F, player, world);
  REQUIRE(game.elapsedSeconds() > 30);
  REQUIRE(game.spawnEnemy({16, 1.45F, 23}, player, world, EnemyKind::Golem));
  game.reset();
  REQUIRE(game.level() == ArenaLevel::Night);
  REQUIRE(game.elapsedSeconds() == 0);
  REQUIRE(game.enemies().empty());
  game.reset(ArenaLevel::Day);
  REQUIRE(game.level() == ArenaLevel::Day);
  REQUIRE(game.elapsedSeconds() == 0);
  game.update(.1F, player, world);
  const float elapsed = game.elapsedSeconds();
  game.update(0, player, world);
  REQUIRE(game.elapsedSeconds() == elapsed);
}
TEST_CASE("Upgrade pause does not advance the encounter timeline",
          "[enemies]") {
  const auto world = emptyArena();
  Gameplay game;
  game.reset(ArenaLevel::Night);
  for (int kill = 0; kill < 3; ++kill) {
    const int before = game.stats().kills;
    REQUIRE(game.spawnEnemy({16, 1.45F, 22}, player, world));
    for (int i = 0; i < 200 && game.stats().kills == before; ++i)
      game.update(.01F, player, world, game.enemies()[0].position);
    REQUIRE(game.stats().kills == before + 1);
    REQUIRE(game.orbs().size() == 1);
    const auto collector = game.orbs()[0].position;
    game.update(.01F, collector, world);
  }
  REQUIRE(game.stats().phase == Phase::ChoosingUpgrade);
  const float clock = game.elapsedSeconds(),
              remaining = game.stats().remainingSeconds;
  for (int i = 0; i < 50; ++i) game.update(.1F, player, world);
  REQUIRE(game.elapsedSeconds() == clock);
  REQUIRE(game.stats().remainingSeconds == remaining);
  REQUIRE(game.chooseUpgrade(0));
  game.update(.1F, player, world);
  REQUIRE(game.elapsedSeconds() == Catch::Approx(clock + .1F));
  REQUIRE(game.level() == ArenaLevel::Night);
}
TEST_CASE("All production level entries accommodate each enemy body",
          "[enemies]") {
  for (auto level : {ArenaLevel::Day, ArenaLevel::Night}) {
    const ArenaWorld world(craft::world::makeArenaLayout(level));
    for (auto kind : {EnemyKind::Zombie, EnemyKind::Golem, EnemyKind::Spider}) {
      Gameplay game;
      for (const auto& entry : world.enemySpawnCandidates()) {
        const auto p = enemyProfile(kind);
        REQUIRE(world.canOccupy({entry.x, p.centreY, entry.z}, p.halfExtents));
        REQUIRE(game.spawnEnemy(entry, world.playerSpawn(), world, kind));
      }
    }
  }
}
TEST_CASE("Mixed waves keep the existing cap and terrain slowdown",
          "[enemies]") {
  const auto world = emptyArena();
  Gameplay game;
  for (int i = 0; i < 32; ++i) {
    const auto kind = static_cast<EnemyKind>(i % 3);
    const bool accepted = game.spawnEnemy(
        {2.5F + (i % 8), 1.45F, 2.5F + (i / 8)}, player, world, kind);
    REQUIRE(accepted == (i < 24));
  }
  REQUIRE(game.enemies().size() == 24);
  craft::world::ArenaLayout layout;
  layout.waterCells = {{16, 24}};
  const ArenaWorld wet(layout);
  for (auto kind : {EnemyKind::Zombie, EnemyKind::Golem, EnemyKind::Spider}) {
    Gameplay swimmer;
    const glm::vec3 target(16.5F, kActorCentreY, 16.5F);
    REQUIRE(swimmer.spawnEnemy({16.5F, 1.45F, 24.5F}, target, wet, kind));
    const auto start = swimmer.enemies()[0].position;
    swimmer.update(.1F, target, wet);
    REQUIRE(glm::length(swimmer.enemies()[0].position - start) ==
            Catch::Approx(enemyProfile(kind).speed * .1F *
                          ArenaWorld::kWaterMovementSpeedMultiplier)
                .margin(.001));
  }
}

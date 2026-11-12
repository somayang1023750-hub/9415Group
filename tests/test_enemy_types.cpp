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
  REQUIRE(g.health == z.health * 3.5F);
  REQUIRE(g.contactDamage == z.contactDamage * 2);
  REQUIRE(g.experience == z.experience * 2);
  REQUIRE(s.health == Catch::Approx(z.health * .75F));
  REQUIRE(s.health == Catch::Approx(3.F));
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
    if (sec <= 30) { REQUIRE(n[0] == 0); REQUIRE(n[2] == 1); }
    if (sec <= 60) REQUIRE(n[1] == 0);
  }
  auto d = enemySpawnWeights(ArenaLevel::Day, 90);
  REQUIRE(d[1] == Catch::Approx(.15));
  d = enemySpawnWeights(ArenaLevel::Day, 120);
  REQUIRE(d[0] == Catch::Approx(.7));
  REQUIRE(d[1] == Catch::Approx(.3));
  auto n = enemySpawnWeights(ArenaLevel::Night, 60);
  REQUIRE(n[0] == Catch::Approx(.25));
  REQUIRE(n[1] == 0);
  REQUIRE(n[2] == Catch::Approx(.75));
  n = enemySpawnWeights(ArenaLevel::Night, 90);
  REQUIRE(n[0] == Catch::Approx(.5));
  REQUIRE(n[1] == Catch::Approx(.1));
  REQUIRE(n[2] == Catch::Approx(.4));
  n = enemySpawnWeights(ArenaLevel::Night, 120);
  REQUIRE(n[0] == Catch::Approx(.5));
  REQUIRE(n[1] == Catch::Approx(.2));
  REQUIRE(n[2] == Catch::Approx(.3));
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
TEST_CASE("Automatic encounters keep a six-block body safety zone",
          "[enemies][spawn]") {
  const auto idleWorld = emptyArena();
  for (auto level : {ArenaLevel::Day, ArenaLevel::Night})
    for (float elapsed : {0.F, 35.F, 65.F, 125.F})
      for (bool allowFarEntry : {false, true}) {
        Gameplay game(5);
        game.reset(level);
        for (int i = 0; i < static_cast<int>(elapsed * 10); ++i)
          game.update(.1F, player, idleWorld);
        craft::world::ArenaLayout layout;
        // Even the 6.2-centre-distance entry clips the body safety zone.
        layout.enemySpawnCandidates = {player, player + glm::vec3(4.5F, 0, 0),
                                       player + glm::vec3(6.2F, 0, 0)};
        if (allowFarEntry)
          // This safe entry would be rejected by the old eight-block zone.
          layout.enemySpawnCandidates.push_back(player + glm::vec3(7.F, 0, 0));
        for (auto& entry : layout.enemySpawnCandidates) entry.y = 1.45F;
        const ArenaWorld world(layout);
        for (int i = 0; i < 210 && game.enemies().empty(); ++i)
          game.update(.01F, player, world);
        REQUIRE(game.enemies().empty() == !allowFarEntry);
        for (const auto& enemy : game.enemies()) {
          const auto p = enemyProfile(enemy.kind);
          auto gap =
              glm::max(glm::abs(enemy.position - player) - p.halfExtents -
                           craft::gameplay::playerHalfExtents(),
                       glm::vec3(0));
          gap.y = 0;
          // The newborn may move once in the same simulation step.
          REQUIRE(glm::length(gap) >=
                  Gameplay::spawnSafetyRadius() - p.speed * .01F - .001F);
        }
      }
}

TEST_CASE("Spawn safety follows the live player at an old entry",
          "[enemies][spawn]") {
  Gameplay game;
  const auto idleWorld = emptyArena();
  game.update(.01F, player, idleWorld);
  craft::world::ArenaLayout layout;
  const glm::vec3 movedPlayer(16.5F, craft::gameplay::kActorCentreY, 3.5F);
  layout.enemySpawnCandidates = {
      movedPlayer, movedPlayer + glm::vec3(4.5F, 0, 0), {28.5F, 1.45F, 28.5F}};
  for (auto& entry : layout.enemySpawnCandidates) entry.y = 1.45F;
  const ArenaWorld world(layout);
  for (int i = 0; i < 210 && game.enemies().empty(); ++i)
    game.update(.01F, movedPlayer, world);
  REQUIRE(game.enemies().size() == 1);
  REQUIRE(glm::length(game.enemies()[0].position -
                      glm::vec3(28.5F, craft::gameplay::kActorCentreY, 28.5F)) <
          .1F);
  REQUIRE(game.stats().health == 5);
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
    "Typed projectile hits preserve profile HP and drop typed experience "
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
    REQUIRE(game.orbs()[0].sourceKind == kind);
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
TEST_CASE(
    "Bodies grazing voxel corners recover instead of retrying a clipped path",
    "[enemies][navigation]") {
  craft::world::ArenaLayout layout;
  layout.obstacles = {{10, 10, 4, 4, 2}};
  const ArenaWorld world(layout);
  const glm::vec3 target(17.5F, kActorCentreY, 16.5F);
  for (auto kind : {EnemyKind::Zombie, EnemyKind::Golem, EnemyKind::Spider})
    for (float edge : {.0002F, .0062F, .0302F})
      for (float z : {13.85F, 13.92F, 14.06F}) {
        const auto p = enemyProfile(kind);
        Gameplay game;
        REQUIRE(game.spawnEnemy({10 - p.halfExtents.x - edge, p.centreY, z},
                                {26, 1.32F, 26}, world, kind));
        for (int frame = 0; frame < 300; ++frame) {
          const auto before = game.enemies()[0].position;
          game.update(.02F, target, world);
          const auto after = game.enemies()[0].position;
          REQUIRE(world.canOccupy(after, p.halfExtents));
          REQUIRE(glm::length(after - before) <= p.speed * .02F + .001F);
        }
        INFO("kind=" << int(kind) << " edge=" << edge << " z=" << z);
        REQUIRE(glm::length(game.enemies()[0].position - target) < 1.4F);
      }
}

TEST_CASE("Side-by-side crowds progress without repeated direction reversals",
          "[enemies][navigation]") {
  const auto world = emptyArena();
  const glm::vec3 staging(3, kActorCentreY, 3);
  for (auto middle : {EnemyKind::Zombie, EnemyKind::Golem, EnemyKind::Spider})
    for (auto sides : {EnemyKind::Zombie, EnemyKind::Golem, EnemyKind::Spider})
      for (float lead : {-.25F, .25F})
        for (float targetX : {14.5F, 18.5F}) {
          INFO("middle=" << int(middle) << " sides=" << int(sides)
                         << " lead=" << lead << " targetX=" << targetX);
          Gameplay game;
          const auto m = enemyProfile(middle), s = enemyProfile(sides);
          REQUIRE(game.spawnEnemy({16.45F, m.centreY, 22.23F}, staging, world,
                                  middle));
          for (float sign : {-1.F, 1.F})
            REQUIRE(game.spawnEnemy(
                {16.45F + sign * (m.halfExtents.x + s.halfExtents.x + .025F),
                 s.centreY, 22.23F + lead},
                staging, world, sides));
          const auto initial = game.enemies();
          std::array<glm::vec3, 3> previousMove{};
          std::array<int, 3> reversals{};
          const glm::vec3 target(targetX, kActorCentreY, 4.5F);
          for (int frame = 0; frame < 350; ++frame) {
            const auto before = game.enemies();
            game.update(.02F, target, world);
            for (std::size_t i = 0; i < game.enemies().size(); ++i) {
              const auto& e = game.enemies()[i];
              const auto p = enemyProfile(e.kind);
              const auto delta = e.position - before[i].position;
              REQUIRE(world.canOccupy(e.position, p.halfExtents));
              REQUIRE(glm::length(delta) <= p.speed * .02F + .001F);
              for (const auto& other : game.enemies())
                if (e.id != other.id) REQUIRE_FALSE(overlap(e, other));
              if (glm::length(delta) > .0001F) {
                if (glm::length(previousMove[i]) > .0001F &&
                    glm::dot(glm::normalize(delta),
                             glm::normalize(previousMove[i])) < -.7F)
                  ++reversals[i];
                previousMove[i] = delta;
              }
            }
          }
          for (std::size_t i = 0; i < initial.size(); ++i) {
            INFO("enemy=" << i << " reversals=" << reversals[i]);
            REQUIRE(reversals[i] <= 2);
            REQUIRE(glm::length(game.enemies()[i].position -
                                initial[i].position) > 5);
          }
        }
}

TEST_CASE("A blocked follower waits steadily and resumes after the queue moves",
          "[enemies][navigation]") {
  craft::world::ArenaLayout layout;
  layout.playerSpawn = {16.5F, 1.45F, 12.5F};
  layout.obstacles = {{15, 10, 1, 13, 2}, {17, 10, 1, 13, 2}};
  const ArenaWorld world(layout);
  Gameplay game;
  const glm::vec3 staging(3, kActorCentreY, 3),
      waitingPlayer(16.5F, kActorCentreY, 12.5F);
  REQUIRE(
      game.spawnEnemy({16.5F, 1.45F, 14.5F}, staging, world, EnemyKind::Golem));
  REQUIRE(game.spawnEnemy({16.5F, 1.45F, 15.3F}, staging, world,
                          EnemyKind::Spider));
  for (int frame = 0; frame < 100; ++frame) {
    const auto previous = game.enemies()[1].position;
    game.update(.02F, waitingPlayer, world);
    if (frame >= 85)
      REQUIRE(glm::length(game.enemies()[1].position - previous) < .001F);
    REQUIRE_FALSE(overlap(game.enemies()[0], game.enemies()[1]));
  }
  const auto stopped = game.enemies();
  const glm::vec3 movedPlayer(16.5F, kActorCentreY, 4.5F);
  for (int frame = 0; frame < 150; ++frame) {
    game.update(.02F, movedPlayer, world);
    for (const auto& e : game.enemies())
      REQUIRE(world.canOccupy(e.position, enemyProfile(e.kind).halfExtents));
    REQUIRE_FALSE(overlap(game.enemies()[0], game.enemies()[1]));
  }
  for (std::size_t i = 0; i < stopped.size(); ++i)
    REQUIRE(stopped[i].position.z - game.enemies()[i].position.z > 3);
}

TEST_CASE("Early Night spawns only Spiders at a slower real cadence", "[enemies][spawn]") {
  craft::world::ArenaLayout layout;
  layout.enemySpawnCandidates={{3.5F,1.45F,3.5F},{28.5F,1.45F,3.5F},{3.5F,1.45F,28.5F},{28.5F,1.45F,28.5F}};
  const ArenaWorld world(layout);
  Gameplay night,day;
  night.reset(ArenaLevel::Night);
  day.reset(ArenaLevel::Day);
  REQUIRE(enemySpawnInterval(ArenaLevel::Night,0)==3);
  REQUIRE(enemySpawnInterval(ArenaLevel::Night,29.99F)==3);
  REQUIRE(enemySpawnInterval(ArenaLevel::Night,30)==2);
  REQUIRE(enemySpawnInterval(ArenaLevel::Day,0)==2);
  for(int i=0;i<28;++i) {night.update(.1F,player,world);day.update(.1F,player,world);}
  REQUIRE(night.enemies().size()==1);
  REQUIRE(day.enemies().size()==2);
  for(int i=0;i<8;++i)night.update(.1F,player,world);
  REQUIRE(night.enemies().size()==2);
  for(const auto& e:night.enemies())REQUIRE(e.kind==EnemyKind::Spider);
  night.reset();night.update(.1F,player,world);
  REQUIRE(night.level()==ArenaLevel::Night);
  REQUIRE(night.enemies().size()==1);
  REQUIRE(night.enemies()[0].kind==EnemyKind::Spider);
}

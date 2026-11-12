#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "core/gameplay/ParticleEffects.h"
#include "world/ArenaWorld.h"
namespace {
using namespace craft::gameplay;
}
TEST_CASE("Bullet debris is short lived and bomb particles expand outwards",
          "[particles]") {
  for (auto weapon : {Weapon::Bullet, Weapon::Bomb}) {
    ParticleEffects fx;
    const glm::vec3 centre(10, 1.32F, 10);
    fx.update(.01F, {{centre, {1, 0, 0}, weapon, false, 1.75F}}, {});
    const auto count = fx.visuals().size();
    REQUIRE(count == (weapon == Weapon::Bullet ? 14 : 64));
    fx.update(.1F, {}, {});
    bool left = false, right = false;
    for (const auto& p : fx.visuals()) {
      REQUIRE(p.position.y >= 1.025F);
      REQUIRE(p.color.a < 1);
      REQUIRE(glm::length(p.position - centre) > .01F);
      left |= p.position.x < centre.x;
      right |= p.position.x > centre.x;
    }
    REQUIRE(left);
    REQUIRE(right);
    for (int i = 0; i < 8; ++i) fx.update(.1F, {}, {});
    REQUIRE(fx.visuals().empty());
  }
}
TEST_CASE("Laser impact arcs distinguish cyan and evolved red", "[particles]") {
  for (bool evolved : {false, true}) {
    ParticleEffects fx;
    fx.update(.01F, {{{10, 1.32F, 10}, {1, 0, 0}, Weapon::Laser, evolved}}, {});
    REQUIRE(fx.visuals().size() == 25);
    for (const auto& p : fx.visuals()) {
      REQUIRE(p.halfExtents.y > p.halfExtents.x);
      REQUIRE(glm::length(p.direction) > .99F);
      if (evolved)
        REQUIRE(p.color.r > p.color.b * 3);
      else
        REQUIRE(p.color.b > p.color.r * 3);
    }
    fx.update(.1F, {}, {});
    REQUIRE_FALSE(fx.visuals().empty());
    fx.update(.1F, {}, {});
    fx.update(.1F, {}, {});
    REQUIRE(fx.visuals().empty());
  }
}
TEST_CASE(
    "Original ground-fire cubes orbit and rise during the three-second burn",
    "[particles]") {
  ParticleEffects fx;
  BurningArea area{{10, 1.32F, 10}, 3.5F};
  fx.update(.01F, {}, {area});
  REQUIRE(fx.visuals().empty());
  REQUIRE(fx.groundFire().size() == 10);
  const auto initial = fx.groundFire();
  float highest = 0;
  for (std::size_t i = 0; i < initial.size(); ++i) {
    const auto& p = initial[i];
    REQUIRE(p.position.y >= 1.16F);
    REQUIRE(p.position.y < 1.88F);
    highest = std::max(highest, p.position.y);
    REQUIRE(p.halfExtents == glm::vec3(.035F));
    REQUIRE(p.color == (i % 2 == 0 ? glm::vec4(1, .18F, .025F, 1)
                                   : glm::vec4(1, .48F, .045F, 1)));
  }
  REQUIRE(highest > 1.45F);
  area.age = .1F;
  fx.update(0, {}, {area});
  REQUIRE(fx.groundFire()[0].position == initial[0].position);
  fx.update(.1F, {}, {area});
  const auto before = initial[0].position - area.position;
  const auto after = fx.groundFire()[0].position - area.position;
  REQUIRE(after.y > before.y);
  // Tangential movement at a fixed radius distinguishes orbiting from rising.
  REQUIRE(before.x * after.z - before.z * after.x > .01F);
  REQUIRE(std::abs(glm::length(glm::vec2(before.x, before.z)) -
                   glm::length(glm::vec2(after.x, after.z))) < .001F);
  area.age = 3;
  fx.update(.1F, {}, {area});
  REQUIRE(fx.groundFire().empty());
}
TEST_CASE("An evolved blast dissipates while the original ground fire persists",
          "[particles]") {
  ParticleEffects fx;
  BurningArea area{{10, 1.32F, 10}, 3.5F};
  const ImpactEvent hit{
      area.position, {0, 1, 0}, Weapon::Bomb, true, area.radius};
  fx.update(.01F, {hit}, {area});
  REQUIRE(fx.visuals().size() == 64);
  REQUIRE(fx.groundFire().size() == 10);
  for (int i = 0; i < 7; ++i) {
    area.age += .1F;
    fx.update(.1F, {}, {area});
  }
  REQUIRE(fx.visuals().empty());
  REQUIRE(fx.groundFire().size() == 10);
  REQUIRE_FALSE(fx.lights().empty());
  const auto frozen = fx.groundFire()[0].position;
  fx.update(0, {}, {area});
  REQUIRE(fx.groundFire()[0].position == frozen);
  fx.reset();
  REQUIRE(fx.groundFire().empty());
  REQUIRE(fx.lights().empty());
}
TEST_CASE("Visual buffers are bounded and reset clears and reseeds effects",
          "[particles]") {
  ParticleEffects fx;
  const ImpactEvent hit{{10, 1.32F, 10}, {1, 0, 0}, Weapon::Bomb, true, 3.5F};
  fx.update(.01F, {hit}, {});
  const auto first = fx.visuals()[0].halfExtents;
  fx.reset();
  REQUIRE(fx.visuals().empty());
  fx.update(.01F, {hit}, {});
  REQUIRE(fx.visuals()[0].halfExtents == first);
  std::vector<ImpactEvent> hits(1000, hit);
  std::vector<BurningArea> areas(1000, {{10, 1.32F, 10}, 3.5F});
  fx.update(.01F, hits, {});
  REQUIRE(fx.visuals().size() <= ParticleEffects::kParticleLimit);
  fx.update(.01F, {}, areas);
  REQUIRE(fx.visuals().size() + fx.groundFire().size() <=
          ParticleEffects::kVisualLimit);
}
TEST_CASE("Wall contacts emit one transient visual impact and no enemy damage",
          "[particles]") {
  craft::world::ArenaLayout layout;
  layout.obstacles = {{19, 16, 1, 1, 2}};
  const craft::world::ArenaWorld world(layout);
  Gameplay game;
  const glm::vec3 player(16, kActorCentreY, 16.5F);
  game.update(.01F, player, world, glm::vec3(25, kActorCentreY, 16.5F));
  for (int i = 0; i < 100 && game.impacts().empty(); ++i)
    game.update(.01F, player, world);
  REQUIRE(game.impacts().size() == 1);
  REQUIRE(game.impacts()[0].weapon == Weapon::Bullet);
  REQUIRE_FALSE(game.impacts()[0].evolved);
  REQUIRE(game.hits().empty());
  REQUIRE(game.stats().kills == 0);
  game.update(.01F, player, world);
  REQUIRE(game.impacts().empty());
  game.reset();
  REQUIRE(game.impacts().empty());
}
TEST_CASE(
    "Particle updates do not change gameplay random choices or simulation",
    "[particles]") {
  Gameplay a(5), b(5);
  ParticleEffects fx;
  const craft::world::ArenaWorld world;
  const glm::vec3 player(16, kActorCentreY, 16);
  for (int frame = 0; frame < 100; ++frame) {
    a.update(.02F, player, world, glm::vec3(25, kActorCentreY, 16));
    b.update(.02F, player, world, glm::vec3(25, kActorCentreY, 16));
    fx.update(.02F, a.impacts(), a.burningAreas());
    REQUIRE(a.stats().health == b.stats().health);
    REQUIRE(a.stats().experience == b.stats().experience);
    REQUIRE(a.enemies().size() == b.enemies().size());
    REQUIRE(a.bullets().size() == b.bullets().size());
    for (std::size_t i = 0; i < a.enemies().size(); ++i)
      REQUIRE(a.enemies()[i].position == b.enemies()[i].position);
  }
}
TEST_CASE(
    "Effect lights stay local bounded coloured and expire with the effect",
    "[particles]") {
  ParticleEffects fx;
  fx.update(.01F, {{{10, 1.32F, 10}, {1, 0, 0}, Weapon::Laser, false}}, {});
  REQUIRE(fx.lights().size() == 1);
  REQUIRE(fx.lights()[0].color.b > fx.lights()[0].color.r * 3);
  REQUIRE(fx.lights()[0].radius < 2);
  REQUIRE(fx.lights()[0].strength < .2F);
  const float strength = fx.lights()[0].strength;
  fx.update(.1F, {}, {});
  REQUIRE(fx.lights()[0].strength < strength);
  fx.update(.1F, {}, {});
  REQUIRE(fx.lights().empty());
  std::vector<BurningArea> areas(100, {{10, 1.32F, 10}, 3.5F});
  fx.update(.01F, {}, areas);
  REQUIRE(fx.lights().size() == ParticleEffects::kLightLimit);
  REQUIRE(fx.lights()[0].color.r > fx.lights()[0].color.b * 10);
  fx.reset();
  REQUIRE(fx.lights().empty());
}

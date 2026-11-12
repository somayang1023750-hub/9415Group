#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>

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
    "Firebomb ground vortex follows burning lifetime and pauses without "
    "advancing",
    "[particles]") {
  ParticleEffects fx;
  BurningArea area{{10, 1.32F, 10}, 3.5F};
  fx.update(.01F, {}, {area});
  REQUIRE(fx.visuals().size() == 48);
  const auto initial = fx.visuals();
  for (const auto& p : initial) {
    REQUIRE(p.position.y < 1.08F);
    REQUIRE(p.position.y > 1.02F);
    REQUIRE(p.color.r > p.color.g * 10);
    REQUIRE(glm::length(glm::vec3(p.position.x - 10, 0, p.position.z - 10)) <=
            area.radius);
  }
  area.age = .1F;
  fx.update(0, {}, {area});
  REQUIRE(fx.visuals()[0].position == initial[0].position);
  fx.update(.1F, {}, {area});
  REQUIRE(glm::length(fx.visuals()[0].position - initial[0].position) > .01F);
  area.age = 3;
  fx.update(.1F, {}, {area});
  REQUIRE(fx.visuals().empty());
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
  REQUIRE(fx.visuals().size() <= ParticleEffects::kVisualLimit);
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

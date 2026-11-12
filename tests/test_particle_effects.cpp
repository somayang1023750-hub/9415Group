#include <algorithm>
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
    "Original fire particles rotate and rise for the burning lifetime and "
    "pause without "
    "advancing",
    "[particles]") {
  ParticleEffects fx;
  BurningArea area{{10, 1.32F, 10}, 3.5F};
  fx.update(.01F, {}, {area});
  REQUIRE(fx.visuals().size() == 48);
  const auto initial = fx.visuals();
  float highest = 0;
  for (const auto& p : initial) {
    REQUIRE(p.position.y > 1.08F);
    REQUIRE(p.position.y - p.halfExtents.y >= 1);
    highest = std::max(highest, p.position.y);
    REQUIRE(p.halfExtents.y > p.halfExtents.x);
    REQUIRE(p.direction == glm::vec3(0, 1, 0));
    REQUIRE(p.color.r > p.color.g);
    REQUIRE(p.color.g > p.color.b * 4);
    REQUIRE(glm::length(glm::vec3(p.position.x - 10, 0, p.position.z - 10)) <=
            area.radius);
  }
  REQUIRE(highest > 1.45F);
  area.age = .1F;
  fx.update(0, {}, {area});
  REQUIRE(fx.visuals()[0].position == initial[0].position);
  fx.update(.1F, {}, {area});
  REQUIRE(glm::length(fx.visuals()[0].position - initial[0].position) > .01F);
  const auto before = initial[0].position - area.position;
  const auto after = fx.visuals()[0].position - area.position;
  // A tangential component verifies orbiting, rather than merely rising/fading.
  REQUIRE(before.x * after.z - before.z * after.x < -.01F);
  area.age = 3;
  fx.update(.1F, {}, {area});
  REQUIRE(fx.visuals().empty());
}
TEST_CASE("Evolved bomb retains a warm visible flame trail behind its flight",
          "[particles]") {
  ParticleEffects fx;
  Bullet bomb{{16, 1.32F, 16}, {13, 0, 0}, 2.8F, 4, Weapon::Bomb};
  bomb.evolved = true;
  fx.update(.01F, {}, {}, {bomb});
  REQUIRE(fx.visuals().size() == 16);
  for (const auto& p : fx.visuals()) {
    REQUIRE(p.position.x < bomb.position.x);
    REQUIRE(p.position.x > bomb.position.x - .2F * 13);
    REQUIRE(p.position.y > bomb.position.y);
    REQUIRE(p.color.r > p.color.g);
    REQUIRE(p.color.g > p.color.b * 4);
    REQUIRE(p.color.a > 0);
    REQUIRE(p.color.a < 1);
  }
  const auto frozen = fx.visuals()[0].position;
  bomb.position.x += 1;
  fx.update(0, {}, {}, {bomb});
  REQUIRE(fx.visuals()[0].position == frozen);
  bomb.life = 2.99F;
  fx.update(.01F, {}, {}, {bomb});
  REQUIRE(fx.visuals().empty());  // No trail ahead of launch history.
  bomb.life = 2.8F;
  bomb.evolved = false;
  auto laser = bomb;
  laser.weapon = Weapon::Laser;
  laser.evolved = true;
  fx.update(.01F, {}, {}, {bomb, laser});
  REQUIRE(fx.visuals().empty());
  bomb.evolved = true;
  fx.update(.01F, {}, {}, {bomb});
  fx.reset();
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
  Bullet fire{{16, 1.32F, 16}, {13, 0, 0}, 2.8F, 4, Weapon::Bomb};
  fire.evolved = true;
  fx.update(.01F, {}, {}, std::vector<Bullet>(1000, fire));
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

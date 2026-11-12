#include <catch2/catch_test_macros.hpp>
#include "core/gameplay/Gameplay.h"
#include "world/ArenaWorld.h"
using namespace craft::gameplay;
TEST_CASE("Training range opt-in preserves real contacts without changing normal defaults", "[showcase]") {
  craft::world::ArenaLayout layout;
  layout.enemySpawnCandidates={{3.5F,1.45F,3.5F}};
  craft::world::ArenaWorld world(layout);
  const glm::vec3 player(16,kActorCentreY,16),target(16,kActorCentreY,23);
  Gameplay game;
  REQUIRE_FALSE(game.showcaseEnabled());
  REQUIRE_FALSE(game.configureShowcaseWeapon(Weapon::Bomb,true,4,true));
  game.setShowcaseEnabled(true);
  REQUIRE(game.spawnEnemy(target,player,world));
  const auto original=game.enemies()[0];
  game.update(.01F,player,world,target);
  REQUIRE_FALSE(game.bullets().empty());
  REQUIRE(game.configureShowcaseWeapon(Weapon::Bullet,true,4,true));
  REQUIRE(game.bullets()[0].damage==2);
  REQUIRE_FALSE(game.bullets()[0].evolved);
  REQUIRE(game.configureShowcaseWeapon(Weapon::Laser,true,4,true));
  REQUIRE(game.configureShowcaseWeapon(Weapon::Bomb,true,4,true));
  REQUIRE_FALSE(game.configureShowcaseWeapon(Weapon::Bomb,true,1,true));
  bool contact=false,fire=false;
  for(int i=0;i<2200;++i) {
    game.update(.1F,player,world,target);
    contact|=!game.impacts().empty();fire|=!game.burningAreas().empty();
    REQUIRE(game.enemies().size()==1);
    REQUIRE(game.enemies()[0].position==original.position);
    REQUIRE(game.enemies()[0].health==original.health);
  }
  REQUIRE(contact);REQUIRE(fire);
  REQUIRE(game.stats().kills==0);REQUIRE(game.orbs().empty());
  REQUIRE(game.stats().health==5);
  REQUIRE(game.stats().remainingSeconds==180);
  REQUIRE(game.stats().phase==Phase::Playing);
  game.update(.1F,{16,kActorCentreY,22.3F},world);
  REQUIRE(game.stats().health==5);
  game.reset();REQUIRE(game.showcaseEnabled());REQUIRE(game.enemies().empty());
  Gameplay normal;
  REQUIRE_FALSE(normal.showcaseEnabled());
  normal.update(.1F,player,world);
  REQUIRE_FALSE(normal.enemies().empty());
  REQUIRE(normal.stats().remainingSeconds<180);
}

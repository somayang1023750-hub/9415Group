#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <optional>
#include <random>
#include <vector>

#include "core/gameplay/ActorGeometry.h"

namespace craft::world {
class ArenaWorld;
}
namespace craft::gameplay {

enum class Weapon { Balance, Laser, Bomb };
struct WeaponProfile {
  int baseDamage;
  float shotCooldown;
  float bulletSpeed;
};
// One source for simulation and C's weapon/upgrade UI. Speeds are blocks/sec.
[[nodiscard]] WeaponProfile weaponProfile(Weapon weapon);
struct Enemy {
  std::uint32_t id;
  glm::vec3 position;
  int health = 4;
  std::optional<glm::vec3> waypoint;
};
struct Bullet {
  glm::vec3 position;
  glm::vec3 velocity;
  float life = 2;
  int damage = 1;
  Weapon weapon = Weapon::Balance;
  int reflections = 0;
  std::vector<std::uint32_t> hitEnemies;
};
struct Explosion {
  glm::vec3 position;
  float radius;
  float age = 0;
};
struct ExperienceOrb {
  glm::vec3 position;
  int value = 1;
};
enum class Phase { Playing, ChoosingUpgrade, Won, Lost };
struct HitEvent {
  glm::vec3 position;
  std::uint32_t enemyId;
  bool killed;
  Weapon weapon;
};
struct ShotEvent {
  glm::vec3 position;
  glm::vec3 direction;
  Weapon weapon;
};
struct Stats {
  int health = 5;
  int experience = 0;
  int level = 1;
  int kills = 0;
  float remainingSeconds = 180;
  Weapon weapon = Weapon::Balance;
  Phase phase = Phase::Playing;
};

// Member A owns simulation. B supplies live map collision/spawn candidates;
// C consumes these read-only snapshots for models, animation, HUD and
// instancing. All actor centres use world X/Z with Y=kActorCentreY; render
// proxies match these bounds.
class Gameplay {
 public:
  explicit Gameplay(std::uint32_t seed = 9415);
  void reset();
  void update(float dt, const glm::vec3& player, const world::ArenaWorld& world,
              std::optional<glm::vec3> shotTarget = std::nullopt);
  bool spawnEnemy(const glm::vec3& position, const glm::vec3& player,
                  const world::ArenaWorld& world);
  bool chooseUpgrade(std::size_t choice);
  [[nodiscard]] int experienceToNextLevel() const { return stats_.level * 3; }
  [[nodiscard]] const Stats& stats() const { return stats_; }
  [[nodiscard]] const std::vector<Enemy>& enemies() const { return enemies_; }
  [[nodiscard]] const std::vector<Bullet>& bullets() const { return bullets_; }
  [[nodiscard]] const std::vector<ExperienceOrb>& orbs() const { return orbs_; }
  [[nodiscard]] const std::vector<Weapon>& upgradeChoices() const {
    return choices_;
  }
  // Frame-local events: C can trigger recoil/hit animation, A's M3 particles
  // can consume position/weapon information. Read before the next update.
  [[nodiscard]] const std::vector<HitEvent>& hits() const { return hits_; }
  [[nodiscard]] const std::vector<ShotEvent>& shots() const { return shots_; }
  // C: short-lived blast rings, separate from M3 particle effects.
  [[nodiscard]] const std::vector<Explosion>& explosions() const {
    return explosions_;
  }
  static constexpr float enemySpeed() { return 4.3F; }  // player default: 5
  static constexpr float blastRadius() { return 3.5F; }
  static glm::vec3 enemyHalfExtents() { return gameplay::enemyHalfExtents(); }

 private:
  void offerUpgrade();
  void damageEnemy(Enemy& enemy, int damage, const glm::vec3& impact,
                   Weapon weapon);
  void explode(const glm::vec3& position, int damage);
  Stats stats_;
  std::vector<Enemy> enemies_;
  std::vector<Explosion> explosions_;
  std::vector<Bullet> bullets_;
  std::vector<ExperienceOrb> orbs_;
  std::vector<HitEvent> hits_;
  std::vector<ShotEvent> shots_;
  std::vector<Weapon> choices_;
  std::uint32_t seed_, nextId_ = 1;
  std::mt19937 random_;
  float spawnClock_ = 0, shotClock_ = 0, hurtClock_ = 0;
};
const char* weaponName(Weapon weapon);
const char* weaponDescription(Weapon weapon);
}  // namespace craft::gameplay

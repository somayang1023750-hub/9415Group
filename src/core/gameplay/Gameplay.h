#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <optional>
#include <random>
#include <vector>

#include "core/gameplay/EnemyTypes.h"

namespace craft::world {
class ArenaWorld;
}
namespace craft::gameplay {

enum class Weapon { Bullet, Balance = Bullet, Laser, Bomb };
enum class UpgradeKind { Unlock, Upgrade, Evolution };
inline constexpr int kMaximumWeaponUpgrades = 4;
struct WeaponState {
  Weapon weapon = Weapon::Bullet;
  int upgrades = 0;
  bool evolved = false;
};
struct UpgradeChoice {
  Weapon weapon = Weapon::Bullet;
  UpgradeKind kind = UpgradeKind::Upgrade;
  int stage = 0;
};
struct WeaponProfile {
  int baseDamage;
  float shotCooldown;
  float bulletSpeed;
  int maxReflections = 0;
  bool piercesEnemies = false;
  float blastRadius = 0.0F;
};
// One source for simulation and C's weapon/upgrade UI. Speeds are blocks/sec.
[[nodiscard]] WeaponProfile weaponProfile(Weapon weapon);
[[nodiscard]] WeaponProfile weaponProfile(Weapon weapon, int upgrades);
struct Enemy {
  std::uint32_t id;
  glm::vec3 position;
  float health = 4;
  std::optional<glm::vec3> waypoint;
  EnemyKind kind = EnemyKind::Zombie;
  float blockedSeconds = 0;
};
struct Bullet {
  glm::vec3 position;
  glm::vec3 velocity;
  float life = 2;
  int damage = 1;
  Weapon weapon = Weapon::Bullet;
  int reflections = 0;
  std::vector<std::uint32_t> hitEnemies;
  float renderSize = 0.10F;
  int maxReflections = 0;
  bool piercesEnemies = false;
  bool evolved = false;
  float blastRadius = 0.0F;
};
struct Explosion {
  glm::vec3 position;
  float radius;
  float age = 0;
};
struct BurningArea {
  glm::vec3 position;
  float radius;
  float age = 0.0F;
  float nextDamageTime = 1.0F;
};
struct ExperienceOrb {
  glm::vec3 position;
  int value = 1;
  // C: pickup appearance can depend on source without changing EXP/magnet rules.
  EnemyKind sourceKind = EnemyKind::Zombie;
};
enum class Phase { Playing, ChoosingUpgrade, Won, Lost };
struct HitEvent {
  glm::vec3 position;
  std::uint32_t enemyId;
  bool killed;
  Weapon weapon;
};
// Presentation-only impacts. One event per actual bullet/laser contact or
// bomb detonation; evolved state is captured from the projectile at launch.
struct ImpactEvent {
  glm::vec3 position;
  glm::vec3 direction;
  Weapon weapon;
  bool evolved = false;
  float radius = 0;
};
struct ShotEvent {
  glm::vec3 position;
  glm::vec3 direction;
  Weapon weapon;
};
struct EnemyAttackEvent {
  std::uint32_t enemyId;
  glm::vec3 position;
  glm::vec3 direction;
};
struct Stats {
  int health = 5;
  int experience = 0;
  int level = 1;
  int kills = 0;
  float remainingSeconds = 180;
  // Most recently selected weapon; full owned arsenal is exposed below.
  Weapon weapon = Weapon::Bullet;
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
  void reset(world::ArenaLevel level);
  [[nodiscard]] world::ArenaLevel level() const { return level_; }
  [[nodiscard]] float elapsedSeconds() const { return elapsedSeconds_; }
  void update(float dt, const glm::vec3& player, const world::ArenaWorld& world,
              std::optional<glm::vec3> shotTarget = std::nullopt);
  bool spawnEnemy(const glm::vec3& position, const glm::vec3& player,
                  const world::ArenaWorld& world,
                  EnemyKind kind = EnemyKind::Zombie);
  bool chooseUpgrade(std::size_t choice);
  [[nodiscard]] int experienceToNextLevel() const { return stats_.level * 3; }
  [[nodiscard]] const Stats& stats() const { return stats_; }
  [[nodiscard]] const std::vector<Enemy>& enemies() const { return enemies_; }
  [[nodiscard]] const std::vector<Bullet>& bullets() const { return bullets_; }
  [[nodiscard]] const std::vector<ExperienceOrb>& orbs() const { return orbs_; }
  [[nodiscard]] const std::vector<Weapon>& upgradeChoices() const {
    return offeredWeaponChoices_;
  }
  [[nodiscard]] const std::vector<UpgradeChoice>& upgradeOptions() const {
    return choices_;
  }
  [[nodiscard]] const std::vector<WeaponState>& weapons() const {
    return weapons_;
  }
  [[nodiscard]] int weaponCount(Weapon weapon) const;
  // C: frame-local hit/impact presentation events. Consume once before the next
  // simulation update; impacts also include wall contacts and launch evolution.
  [[nodiscard]] const std::vector<HitEvent>& hits() const { return hits_; }
  [[nodiscard]] const std::vector<ImpactEvent>& impacts() const {
    return impacts_;
  }
  [[nodiscard]] const std::vector<ShotEvent>& shots() const { return shots_; }
  // Frame-local enemy attacks: emitted when proximity damage is applied; read
  // before the next update, like hits() and shots().
  [[nodiscard]] const std::vector<EnemyAttackEvent>& attacks() const {
    return attacks_;
  }
  // C: short-lived blast rings, separate from M3 particle effects.
  [[nodiscard]] const std::vector<Explosion>& explosions() const {
    return explosions_;
  }
  [[nodiscard]] const std::vector<BurningArea>& burningAreas() const {
    return burningAreas_;
  }
  static constexpr float enemySpeed() { return 3.3F; }  // player default: 5
  static constexpr float blastRadius() { return 1.75F; }
  static glm::vec3 enemyHalfExtents() { return gameplay::enemyHalfExtents(); }

 private:
  void offerUpgrade();
  void damageEnemy(Enemy& enemy, int damage, const glm::vec3& impact,
                   Weapon weapon);
  void explode(const glm::vec3& position, int damage, float blastRadius,
               bool firebomb);
  world::ArenaLevel level_ = world::ArenaLevel::Day;
  Stats stats_;
  std::vector<Enemy> enemies_;
  std::vector<Explosion> explosions_;
  std::vector<BurningArea> burningAreas_;
  std::vector<Bullet> bullets_;
  std::vector<ExperienceOrb> orbs_;
  std::vector<HitEvent> hits_;
  std::vector<ImpactEvent> impacts_;
  std::vector<ShotEvent> shots_;
  std::vector<EnemyAttackEvent> attacks_;
  std::vector<UpgradeChoice> choices_;
  std::vector<Weapon> offeredWeaponChoices_;
  std::vector<WeaponState> weapons_{{Weapon::Bullet, 0, false}};
  std::vector<float> weaponShotClocks_{0.0F};
  std::uint32_t seed_, nextId_ = 1;
  std::mt19937 random_;
  float spawnClock_ = 0, hurtClock_ = 0, elapsedSeconds_ = 0;
};
const char* weaponName(Weapon weapon);
const char* weaponDescription(Weapon weapon);
const char* upgradeName(const UpgradeChoice& choice);
const char* upgradeDescription(const UpgradeChoice& choice);
}  // namespace craft::gameplay

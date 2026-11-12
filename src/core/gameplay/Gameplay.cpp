#include "core/gameplay/Gameplay.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/geometric.hpp>
#include <limits>
#include <queue>
#include <utility>

#include "world/ArenaWorld.h"

namespace craft::gameplay {
namespace {
constexpr float kEnemySpawnIntervalSeconds = 2.0F;
constexpr float kSpawnBatchIncreaseSeconds = 30.0F;
constexpr float radius = 0.10F;
float distance(const glm::vec3& a, const glm::vec3& b) {
  return glm::length(glm::vec3(a.x - b.x, 0, a.z - b.z));
}
bool overlaps(const glm::vec3& a, const glm::vec3& b, float size) {
  return std::abs(a.x - b.x) < size && std::abs(a.z - b.z) < size;
}
// Swept segment versus expanded AABB: prevents fast bullets tunnelling through
// thin cover/enemies and lets the nearest collision win (walls win ties).
float intersection(glm::vec3 from, glm::vec3 to, glm::vec3 lo, glm::vec3 hi,
                   glm::vec3* normal = nullptr) {
  if (normal) *normal = glm::vec3(0);
  float enter = 0, leave = 1;
  for (int axis = 0; axis < 3; ++axis) {
    const float d = to[axis] - from[axis];
    if (std::abs(d) < 1e-7F) {
      if (from[axis] < lo[axis] || from[axis] > hi[axis]) return 2;
    } else {
      float a = (lo[axis] - from[axis]) / d, b = (hi[axis] - from[axis]) / d;
      if (a > b) std::swap(a, b);
      if (normal && a >= enter - 0.00001F) {
        if (a > enter + 0.00001F) *normal = glm::vec3(0);
        (*normal)[axis] = d > 0 ? -1.F : 1.F;
      }
      enter = std::max(enter, a);
      leave = std::min(leave, b);
      if (enter > leave) return 2;
    }
  }
  return enter;
}
}  // namespace
Gameplay::Gameplay(std::uint32_t seed) : seed_(seed), random_(seed) {}
void Gameplay::reset() {
  stats_ = {};
  enemies_.clear();
  explosions_.clear();
  burningAreas_.clear();
  bullets_.clear();
  orbs_.clear();
  hits_.clear();
  shots_.clear();
  attacks_.clear();
  choices_.clear();
  offeredWeaponChoices_.clear();
  weapons_ = {{Weapon::Bullet, 0, false}};
  weaponShotClocks_ = {0.0F};
  nextId_ = 1;
  random_.seed(seed_);
  spawnClock_ = hurtClock_ = 0;
  elapsedSeconds_ = 0.0F;
}
bool Gameplay::spawnEnemy(const glm::vec3& metadata, const glm::vec3& player,
                          const world::ArenaWorld& world) {
  if (!std::isfinite(metadata.y)) return false;
  const glm::vec3 p(metadata.x, kActorCentreY, metadata.z);
  if (enemies_.size() >= 24 || distance(p, player) < 4 ||
      !world.canOccupy(p, enemyHalfExtents()))
    return false;
  for (const auto& e : enemies_)
    if (overlaps(p, e.position, enemySeparation() + 0.02F)) return false;
  enemies_.push_back({nextId_++, p});
  return true;
}
void Gameplay::offerUpgrade() {
  if (stats_.experience < experienceToNextLevel()) return;
  std::vector<UpgradeChoice> available;
  for (const auto& state : weapons_) {
    if (state.evolved) continue;
    if (state.upgrades < kMaximumWeaponUpgrades)
      available.push_back({state.weapon, UpgradeKind::Upgrade,
                           state.upgrades + 1});
    else
      available.push_back({state.weapon, UpgradeKind::Evolution,
                           kMaximumWeaponUpgrades + 1});
  }
  for (const auto weapon : {Weapon::Laser, Weapon::Bomb})
    if (weaponCount(weapon) == 0)
      available.push_back({weapon, UpgradeKind::Unlock, 0});

  if (available.empty()) {
    while (stats_.experience >= experienceToNextLevel()) {
      stats_.experience -= experienceToNextLevel();
      ++stats_.level;
      stats_.health = 5;
    }
    return;
  }

  std::shuffle(available.begin(), available.end(), random_);
  const std::size_t choiceCount = std::min<std::size_t>(2, available.size());
  choices_.assign(available.begin(), available.begin() + choiceCount);
  offeredWeaponChoices_.clear();
  offeredWeaponChoices_.reserve(choiceCount);
  for (const auto& option : choices_)
    offeredWeaponChoices_.push_back(option.weapon);
  stats_.phase = Phase::ChoosingUpgrade;
}
bool Gameplay::chooseUpgrade(std::size_t choice) {
  if (stats_.phase != Phase::ChoosingUpgrade || choice >= choices_.size())
    return false;
  const UpgradeChoice option = choices_[choice];
  auto state = std::find_if(weapons_.begin(), weapons_.end(),
                            [&](const WeaponState& candidate) {
                              return candidate.weapon == option.weapon;
                            });
  if (option.kind == UpgradeKind::Unlock) {
    if (state != weapons_.end()) return false;
    weapons_.push_back({option.weapon, 0, false});
    weaponShotClocks_.push_back(0.0F);
  } else {
    if (state == weapons_.end()) return false;
    if (option.kind == UpgradeKind::Upgrade) {
      if (state->evolved || state->upgrades >= kMaximumWeaponUpgrades ||
          option.stage != state->upgrades + 1)
        return false;
      ++state->upgrades;
    } else if (option.kind == UpgradeKind::Evolution) {
      if (state->evolved || state->upgrades != kMaximumWeaponUpgrades ||
          option.stage != kMaximumWeaponUpgrades + 1)
        return false;
      state->evolved = true;
    }
  }
  stats_.weapon = option.weapon;
  stats_.experience -= experienceToNextLevel();
  ++stats_.level;
  stats_.health = 5;  // Every accepted level-up fully restores player health.
  choices_.clear();
  offeredWeaponChoices_.clear();
  stats_.phase = Phase::Playing;
  offerUpgrade();
  return true;
}
void Gameplay::update(float dt, const glm::vec3& player,
                      const world::ArenaWorld& world,
                      std::optional<glm::vec3> target) {
  hits_.clear();
  shots_.clear();
  attacks_.clear();
  if (!std::isfinite(dt) || dt <= 0 || stats_.phase != Phase::Playing) return;
  dt = std::min(dt, 0.1F);
  elapsedSeconds_ += dt;
  for (auto& blast : explosions_) blast.age += dt;
  explosions_.erase(
      std::remove_if(explosions_.begin(), explosions_.end(),
                     [](const Explosion& blast) { return blast.age >= 0.4F; }),
      explosions_.end());
  stats_.remainingSeconds = std::max(0.F, stats_.remainingSeconds - dt);
  for (float& clock : weaponShotClocks_)
    clock = std::max(0.0F, clock - dt);
  hurtClock_ = std::max(0.F, hurtClock_ - dt);
  spawnClock_ -= dt;
  if (spawnClock_ <= 0) {
    const auto& candidates = world.enemySpawnCandidates();
    if (!candidates.empty()) {
      const std::size_t start = random_() % candidates.size();
      const std::size_t batchSize =
          1 + static_cast<std::size_t>(elapsedSeconds_ /
                                       kSpawnBatchIncreaseSeconds);
      std::size_t spawned = 0;
      for (std::size_t i = 0;
           i < candidates.size() && spawned < batchSize; ++i) {
        if (spawnEnemy(candidates[(start + i) % candidates.size()], player,
                       world))
          ++spawned;
      }
    }
    spawnClock_ = kEnemySpawnIntervalSeconds;
  }
  if (target && std::isfinite(target->x) && std::isfinite(target->z)) {
    glm::vec3 direction{target->x - player.x, 0, target->z - player.z};
    if (glm::length(direction) > 0.001F) {
      const glm::vec3 normalizedDirection = glm::normalize(direction);
      for (std::size_t i = 0; i < weapons_.size(); ++i) {
        if (weaponShotClocks_[i] > 0.0F) continue;
        const WeaponState& state = weapons_[i];
        const auto profile = weaponProfile(state.weapon, state.upgrades);
        const auto emitBullet = [&](const glm::vec3& shotDirection, int damage,
                                    float renderSize) {
          Bullet bullet{player, shotDirection * profile.bulletSpeed, 3.0F,
                        damage, state.weapon};
          bullet.renderSize = renderSize;
          bullet.maxReflections = profile.maxReflections;
          bullet.piercesEnemies =
              profile.piercesEnemies ||
              (state.weapon == Weapon::Laser && state.evolved);
          bullet.evolved = state.evolved;
          bullet.blastRadius = profile.blastRadius;
          bullets_.push_back(std::move(bullet));
          shots_.push_back({player, shotDirection, state.weapon});
        };

        emitBullet(normalizedDirection, profile.baseDamage, 0.10F);
        if (state.weapon == Weapon::Bullet && state.evolved) {
          constexpr float spreadRadians = 0.523598776F;
          const int sideDamage = weaponProfile(Weapon::Bullet).baseDamage;
          for (const float angle : {-spreadRadians, spreadRadians}) {
            const float cosine = std::cos(angle);
            const float sine = std::sin(angle);
            const glm::vec3 sideDirection{
                normalizedDirection.x * cosine + normalizedDirection.z * sine,
                0.0F,
                -normalizedDirection.x * sine + normalizedDirection.z * cosine};
            emitBullet(sideDirection, sideDamage, 0.065F);
          }
        }
        weaponShotClocks_[i] = profile.shotCooldown;
      }
    }
  }
  // Replan around live actor occupancy, rather than forcing every enemy into
  // the same static flow-field lane. Keep each waypoint until its centre is
  // reached: recomputing it on crossing a cell boundary caused turn
  // oscillation.
  constexpr int width = world::ArenaWorld::kWidth,
                depth = world::ArenaWorld::kDepth;
  const auto worldClear = [&](const glm::vec3& from, const glm::vec3& to) {
    const int count =
        std::max(1, static_cast<int>(std::ceil(distance(from, to) / 0.1F)));
    for (int i = 1; i <= count; ++i)
      if (!world.canOccupy(from + (to - from) * (static_cast<float>(i) / count),
                           enemyHalfExtents()))
        return false;
    return true;
  };
  for (auto& e : enemies_) {
    const auto actorClear = [&](const glm::vec3& point) {
      for (const auto& other : enemies_)
        if (other.id != e.id &&
            overlaps(point, other.position, enemySeparation()))
          return false;
      return true;
    };
    const auto actorRouteClear = [&](const glm::vec3& from,
                                     const glm::vec3& to) {
      const int count =
          std::max(1, static_cast<int>(std::ceil(distance(from, to) / 0.1F)));
      for (int i = 1; i <= count; ++i)
        if (!actorClear(from + (to - from) * (static_cast<float>(i) / count)))
          return false;
      return true;
    };
    if (e.waypoint && distance(e.position, *e.waypoint) < 0.02F)
      e.waypoint.reset();
    if (!e.waypoint) {
      const int x = static_cast<int>(std::floor(e.position.x));
      const int z = static_cast<int>(std::floor(e.position.z));
      if (x < 0 || x >= width || z < 0 || z >= depth) continue;
      const int start = z * width + x;
      std::array<int, width * depth> parent;
      parent.fill(-1);
      std::queue<int> pending;
      pending.push(start);
      parent[start] = start;
      int destination = -1;
      constexpr int steps[][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
      while (!pending.empty()) {
        const int cell = pending.front();
        pending.pop();
        const glm::vec3 centre(cell % width + 0.5F, kActorCentreY,
                               cell / width + 0.5F);
        if (distance(centre, player) < 1.6F && worldClear(centre, player) &&
            actorRouteClear(centre, player)) {
          destination = cell;
          break;
        }
        // Rotate tie order per actor to spread equal-length routes.
        for (int offset = 0; offset < 4; ++offset) {
          const auto& step = steps[(offset + e.id) % 4];
          int nx = cell % width + step[0], nz = cell / width + step[1];
          if (nx < 0 || nx >= width || nz < 0 || nz >= depth) continue;
          int next = nz * width + nx;
          glm::vec3 point(nx + 0.5F, kActorCentreY, nz + 0.5F);
          if (parent[next] >= 0 ||
              !world.canOccupy(point, enemyHalfExtents()) || !actorClear(point))
            continue;
          parent[next] = cell;
          pending.push(next);
        }
      }
      if (destination >= 0) {
        if (destination == start)
          e.waypoint = player;
        else {
          int next = destination;
          while (parent[next] != start) next = parent[next];
          const glm::vec3 centre(x + 0.5F, kActorCentreY, z + 0.5F);
          const glm::vec3 goal(next % width + 0.5F, kActorCentreY,
                               next / width + 0.5F);
          // Only centre before a turn when required by solid geometry.
          e.waypoint = worldClear(e.position, goal) ? goal : centre;
        }
      }
    }
    if (e.waypoint) {
      glm::vec3 delta = *e.waypoint - e.position;
      delta.y = 0;
      const float length = glm::length(delta);
      if (length > 0.001F) {
        const int currentCellX =
            static_cast<int>(std::floor(e.position.x));
        const int currentCellZ =
            static_cast<int>(std::floor(e.position.z));
        const float terrainSpeed =
            world.isWaterSurface(currentCellX, currentCellZ)
                ? world::ArenaWorld::kWaterMovementSpeedMultiplier
                : 1.0F;
        const float travel =
            std::min(length, enemySpeed() * terrainSpeed * dt);
        const int count =
            std::max(1, static_cast<int>(std::ceil(travel / 0.08F)));
        const glm::vec3 step = delta / length * (travel / count);
        for (int i = 0; i < count; ++i) {
          const auto next = e.position + step;
          if (!world.canOccupy(next, enemyHalfExtents()) ||
              overlaps(next, player, playerEnemySeparation()) ||
              !actorClear(next)) {
            e.waypoint.reset();
            break;
          }
          e.position = next;
        }
      }
    }
    if (distance(e.position, player) < 0.82F && hurtClock_ <= 0) {
      --stats_.health;
      hurtClock_ = 1;
      glm::vec3 direction{player.x - e.position.x, 0.0F,
                          player.z - e.position.z};
      const float directionLength = glm::length(direction);
      if (directionLength > 0.0001F)
        direction /= directionLength;
      else
        direction = {0.0F, 0.0F, 1.0F};
      attacks_.push_back({e.id, e.position, direction});
    }
  }
  for (auto& area : burningAreas_) {
    area.age += dt;
    while (area.nextDamageTime <= area.age + 0.0001F &&
           area.nextDamageTime <= 3.0F) {
      for (auto& enemy : enemies_)
        if (enemy.health > 0 &&
            distance(area.position, enemy.position) <= area.radius)
          damageEnemy(enemy, 2, enemy.position, Weapon::Bomb);
      area.nextDamageTime += 1.0F;
    }
  }
  burningAreas_.erase(
      std::remove_if(burningAreas_.begin(), burningAreas_.end(),
                     [](const BurningArea& area) { return area.age >= 3.0F; }),
      burningAreas_.end());
  for (auto& b : bullets_) {
    float remaining = std::min(dt, b.life);
    b.life -= dt;
    bool consumed = false;
    // Bounded by 24 unique enemy hits + one reflection + terminal wall impact.
    for (int contact = 0; contact < 28 && remaining > 0.000001F && !consumed;
         ++contact) {
      const auto end = b.position + b.velocity * remaining;
      float closest = 2;
      Enemy* victim = nullptr;
      glm::vec3 wallNormal(0);
      const glm::vec3 padding(radius);
      const auto low = glm::floor(glm::min(b.position, end) - padding);
      const auto high = glm::floor(glm::max(b.position, end) + padding);
      for (int z = static_cast<int>(low.z); z <= static_cast<int>(high.z); ++z)
        for (int y = static_cast<int>(low.y); y <= static_cast<int>(high.y);
             ++y)
          for (int x = static_cast<int>(low.x); x <= static_cast<int>(high.x);
               ++x)
            if (world.isSolid(x, y, z)) {
              glm::vec3 normal;
              const float t = intersection(
                  b.position, end, glm::vec3(x, y, z) - padding,
                  glm::vec3(x + 1, y + 1, z + 1) + padding, &normal);
              if (t < closest - 0.00001F) {
                closest = t;
                wallNormal = normal;
              } else if (t <= 1 && std::abs(t - closest) < 0.00001F) {
                for (int axis = 0; axis < 3; ++axis)
                  if (normal[axis] != 0) wallNormal[axis] = normal[axis];
              }
            }
      bool victimAlreadyHit = false;
      for (auto& e : enemies_) {
        if (e.health <= 0) continue;
        const bool alreadyHit =
            std::find(b.hitEnemies.begin(), b.hitEnemies.end(), e.id) !=
            b.hitEnemies.end();
        const bool blocksAfterReflection =
            b.weapon == Weapon::Laser && !b.piercesEnemies;
        if (alreadyHit && !blocksAfterReflection) continue;
        const float t = intersection(
            b.position, end, e.position - enemyHalfExtents() - padding,
            e.position + enemyHalfExtents() + padding);
        if (t < closest) {
          closest = t;
          victim = &e;
          victimAlreadyHit = alreadyHit;
        }
      }
      if (closest > 1) {
        b.position = end;
        remaining = 0;
        break;
      }
      b.position += (end - b.position) * closest;
      remaining *= 1 - closest;
      if (b.weapon == Weapon::Bomb) {
        explode(b.position, b.damage, b.blastRadius, b.evolved);
        consumed = true;
      } else if (victim) {
        const bool evolvedLaserAfterBounce =
            b.weapon == Weapon::Laser && b.evolved && !b.piercesEnemies;
        if (!victimAlreadyHit || evolvedLaserAfterBounce)
          damageEnemy(*victim, b.damage, b.position, b.weapon);
        if (!victimAlreadyHit) b.hitEnemies.push_back(victim->id);
        if (b.weapon == Weapon::Bullet ||
            (b.weapon == Weapon::Laser && !b.piercesEnemies))
          consumed = true;
      } else if (b.weapon == Weapon::Laser &&
                 b.reflections < b.maxReflections &&
                 glm::length(wallNormal) > 0) {
        for (int axis = 0; axis < 3; ++axis)
          if (wallNormal[axis] != 0) b.velocity[axis] = -b.velocity[axis];
        b.position += wallNormal * 0.002F;
        ++b.reflections;
        if (b.evolved) b.piercesEnemies = false;
      } else
        consumed = true;
    }
    if (!consumed && b.life <= 0 && b.weapon == Weapon::Bomb)
      explode(b.position, b.damage, b.blastRadius, b.evolved);
    if (consumed) b.life = 0;
  }
  enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                                [](const Enemy& e) { return e.health <= 0; }),
                 enemies_.end());
  bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
                                [](const Bullet& b) { return b.life <= 0; }),
                 bullets_.end());
  for (auto& orb : orbs_) {
    const float d = distance(orb.position, player);
    if (d < 4 && d > 0.001F)
      orb.position += (player - orb.position) * (std::min(d, 7.F * dt) / d);
    if (distance(orb.position, player) < 0.65F) {
      stats_.experience += orb.value;
      orb.value = 0;
    }
  }
  orbs_.erase(
      std::remove_if(orbs_.begin(), orbs_.end(),
                     [](const ExperienceOrb& o) { return o.value == 0; }),
      orbs_.end());
  if (stats_.health <= 0)
    stats_.phase = Phase::Lost;
  else if (stats_.remainingSeconds <= 0)
    stats_.phase = Phase::Won;
  else
    offerUpgrade();
}
void Gameplay::damageEnemy(Enemy& enemy, int damage, const glm::vec3& impact,
                           Weapon weapon) {
  if (enemy.health <= 0) return;
  enemy.health -= damage;
  hits_.push_back({impact, enemy.id, enemy.health <= 0, weapon});
  if (enemy.health <= 0) {
    ++stats_.kills;
    orbs_.push_back({enemy.position});
  }
}
void Gameplay::explode(const glm::vec3& position, int damage, float blastRadius,
                       bool firebomb) {
  explosions_.push_back({position, blastRadius});
  if (firebomb) burningAreas_.push_back({position, blastRadius});
  // No falloff: every enemy centre inside the radius receives full damage,
  // including enemies behind cover. The player is not damaged by own bombs.
  for (auto& enemy : enemies_)
    if (distance(position, enemy.position) <= blastRadius)
      damageEnemy(enemy, damage, enemy.position, Weapon::Bomb);
}
int Gameplay::weaponCount(Weapon weapon) const {
  return static_cast<int>(std::count_if(
      weapons_.begin(), weapons_.end(),
      [weapon](const WeaponState& state) { return state.weapon == weapon; }));
}
WeaponProfile weaponProfile(Weapon weapon) {
  return weaponProfile(weapon, 0);
}
WeaponProfile weaponProfile(Weapon weapon, int upgrades) {
  WeaponProfile profile{};
  switch (weapon) {
    case Weapon::Bullet:
      profile = {2, 0.30F, 5.0F};
      break;
    case Weapon::Laser:
      profile = {1, 0.48F, 40.0F, 1, false};
      break;
    case Weapon::Bomb:
      profile = {2, 1.2F, 8.0F, 0, false, Gameplay::blastRadius()};
      break;
    default:
      profile = {2, 0.30F, 5.0F};
      break;
  }
  upgrades = std::clamp(upgrades, 0, kMaximumWeaponUpgrades);
  if (weapon == Weapon::Bullet) {
    if (upgrades >= 1) profile.bulletSpeed *= 2.0F;
    if (upgrades >= 2) profile.baseDamage *= 2;
    if (upgrades >= 3) profile.bulletSpeed *= 2.0F;
    if (upgrades >= 4) profile.shotCooldown *= 0.5F;
  } else if (weapon == Weapon::Laser) {
    if (upgrades >= 1) profile.baseDamage *= 2;
    if (upgrades >= 2) profile.shotCooldown *= 0.5F;
    if (upgrades >= 3) ++profile.maxReflections;
    if (upgrades >= 4) profile.shotCooldown *= 0.5F;
  } else if (weapon == Weapon::Bomb) {
    if (upgrades >= 1) profile.blastRadius *= 2.0F;
    if (upgrades >= 2) profile.bulletSpeed += 5.0F;
    if (upgrades >= 3) profile.baseDamage *= 2;
    if (upgrades >= 4) profile.shotCooldown *= (2.0F / 3.0F);
  }
  return profile;
}
const char* weaponName(Weapon weapon) {
  switch (weapon) {
    case Weapon::Bullet:
      return "Bullet";
    case Weapon::Laser:
      return "Laser";
    case Weapon::Bomb:
      return "Bomb";
  }
  return "Bullet";
}
const char* weaponDescription(Weapon weapon) {
  switch (weapon) {
    case Weapon::Bullet:
      return "Standard bullet. Stops on the first enemy or wall.";
    case Weapon::Laser:
      return "Stops on the first enemy. Reflects off one wall and stops at "
             "the next.";
    case Weapon::Bomb:
      return "Explodes on enemies or walls. Deals 2 damage within a 1.75-block "
             "radius.";
  }
  return "Standard bullet. Stops on the first enemy or wall.";
}
const char* upgradeName(const UpgradeChoice& choice) {
  if (choice.kind == UpgradeKind::Unlock)
    return choice.weapon == Weapon::Laser ? "Unlock Laser" : "Unlock Bomb";
  if (choice.kind == UpgradeKind::Evolution) {
    switch (choice.weapon) {
      case Weapon::Laser:
        return "Laser Evolution";
      case Weapon::Bomb:
        return "Firebomb Evolution";
      case Weapon::Bullet:
        return "Bullet Evolution";
    }
  }
  if (choice.weapon == Weapon::Laser) {
    switch (choice.stage) {
      case 1:
        return "Laser Upgrade 1: Damage x2";
      case 2:
        return "Laser Upgrade 2: Fire rate x2";
      case 3:
        return "Laser Upgrade 3: Ricochet +1";
      case 4:
        return "Laser Upgrade 4: Fire rate x2";
      default:
        return "Laser Upgrade";
    }
  }
  if (choice.weapon == Weapon::Bomb) {
    switch (choice.stage) {
      case 1:
        return "Bomb Upgrade 1: Blast radius x2";
      case 2:
        return "Bomb Upgrade 2: Speed +5";
      case 3:
        return "Bomb Upgrade 3: Damage x2";
      case 4:
        return "Bomb Upgrade 4: Fire rate +50%";
      default:
        return "Bomb Upgrade";
    }
  }
  switch (choice.stage) {
    case 1:
      return "Bullet Upgrade 1: Speed x2";
    case 2:
      return "Bullet Upgrade 2: Damage x2";
    case 3:
      return "Bullet Upgrade 3: Speed x2";
    case 4:
      return "Bullet Upgrade 4: Fire rate x2";
    default:
      return "Weapon Upgrade";
  }
}
const char* upgradeDescription(const UpgradeChoice& choice) {
  if (choice.kind == UpgradeKind::Unlock) {
    if (choice.weapon == Weapon::Laser)
      return "Adds the laser to your arsenal. It will fire alongside your "
             "other ready weapons.";
    return "Adds the bomb to your arsenal. It will fire alongside your other "
           "ready weapons.";
  }
  if (choice.kind == UpgradeKind::Evolution) {
    switch (choice.weapon) {
      case Weapon::Laser:
        return "Turns the beam red. It pierces enemies until its first wall "
               "bounce; after bouncing, the next enemy stops it.";
      case Weapon::Bomb:
        return "Evolves into a red firebomb. It leaves orange-red sparks and "
               "creates a 3-second burning area that deals 2 damage each "
               "second within its blast radius.";
      case Weapon::Bullet:
        return "Fires a central bullet and two small side bullets at -30 and "
               "+30 degrees. Side bullets deal normal bullet damage.";
    }
  }
  if (choice.weapon == Weapon::Laser) {
    switch (choice.stage) {
      case 1:
        return "Doubles laser damage.";
      case 2:
        return "Doubles laser fire rate by halving its firing interval.";
      case 3:
        return "Adds one obstacle ricochet.";
      case 4:
        return "Doubles laser fire rate by halving its firing interval.";
      default:
        return "Improves the laser.";
    }
  }
  if (choice.weapon == Weapon::Bomb) {
    switch (choice.stage) {
      case 1:
        return "Doubles the bomb's explosion radius.";
      case 2:
        return "Adds 5 blocks per second to bomb flight speed.";
      case 3:
        return "Doubles bomb explosion damage.";
      case 4:
        return "Increases bomb fire rate by 50% (reduces its firing interval).";
      default:
        return "Improves the bomb.";
    }
  }
  switch (choice.stage) {
    case 1:
      return "Doubles bullet projectile speed.";
    case 2:
      return "Doubles bullet damage.";
    case 3:
      return "Doubles bullet projectile speed again.";
    case 4:
      return "Doubles bullet fire rate by halving its firing interval.";
    default:
      return "Improves this weapon.";
  }
}
}  // namespace craft::gameplay

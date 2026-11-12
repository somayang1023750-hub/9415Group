#include "core/gameplay/Gameplay.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
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
bool overlaps(const glm::vec3& a, const glm::vec3& ah, const glm::vec3& b,
              const glm::vec3& bh) {
  return std::abs(a.x - b.x) < ah.x + bh.x - 0.00001F &&
         std::abs(a.z - b.z) < ah.z + bh.z - 0.00001F;
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
void Gameplay::reset(world::ArenaLevel level) {
  level_ = level;
  reset();
}
void Gameplay::reset() {
  stats_ = {};
  enemies_.clear();
  explosions_.clear();
  burningAreas_.clear();
  bullets_.clear();
  orbs_.clear();
  hits_.clear();
  impacts_.clear();
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
                          const world::ArenaWorld& world, EnemyKind kind) {
  if (!std::isfinite(metadata.y)) return false;
  const auto profile = enemyProfile(kind);
  const glm::vec3 p(metadata.x, profile.centreY, metadata.z);
  if (enemies_.size() >= 24 || distance(p, player) < 4 ||
      !world.canOccupy(p, profile.halfExtents))
    return false;
  for (const auto& e : enemies_)
    if (overlaps(p, profile.halfExtents + glm::vec3(.01F), e.position,
                 enemyProfile(e.kind).halfExtents))
      return false;
  enemies_.push_back({nextId_++, p, profile.health, std::nullopt, kind});
  return true;
}
void Gameplay::offerUpgrade() {
  if (stats_.experience < experienceToNextLevel()) return;
  std::vector<UpgradeChoice> available;
  for (const auto& state : weapons_) {
    if (state.evolved) continue;
    if (state.upgrades < kMaximumWeaponUpgrades)
      available.push_back(
          {state.weapon, UpgradeKind::Upgrade, state.upgrades + 1});
    else
      available.push_back(
          {state.weapon, UpgradeKind::Evolution, kMaximumWeaponUpgrades + 1});
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
  impacts_.clear();
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
  for (float& clock : weaponShotClocks_) clock = std::max(0.0F, clock - dt);
  hurtClock_ = std::max(0.F, hurtClock_ - dt);
  spawnClock_ -= dt;
  if (spawnClock_ <= 0) {
    const auto& candidates = world.enemySpawnCandidates();
    if (!candidates.empty()) {
      const auto weights = enemySpawnWeights(level_, elapsedSeconds_);
      std::discrete_distribution<int> chooseKind(weights.begin(),
                                                 weights.end());
      const std::size_t batchSize =
          1 + static_cast<std::size_t>(elapsedSeconds_ /
                                       kSpawnBatchIncreaseSeconds);
      for (std::size_t slot = 0; slot < batchSize && enemies_.size() < 24;
           ++slot) {
        const int selected = chooseKind(random_);
        const std::size_t start = random_() % candidates.size();
        bool spawned = false;
        // Try every entry with the requested body's actual clearance. If all
        // are blocked, another eligible type may fill the slot; never introduce
        // night-only spiders into Day or a type whose ramp has not started.
        for (int attempt = 0; attempt < 3 && !spawned; ++attempt) {
          const int index = (selected + attempt) % 3;
          if (weights[index] <= 0) continue;
          const auto kind = static_cast<EnemyKind>(index);
          const auto extents =
              enemyProfile(kind).halfExtents + playerHalfExtents();
          for (std::size_t i = 0; i < candidates.size(); ++i) {
            const auto& entry = candidates[(start + i) % candidates.size()];
            auto gap =
                glm::max(glm::abs(entry - player) - extents, glm::vec3(0));
            gap.y = 0;
            // Never relax the safety zone to fill a batch or to fit a different
            // body. If no safe entry remains, skip this slot until the next
            // tick.
            if (glm::length(gap) < spawnSafetyRadius()) continue;
            if (spawnEnemy(entry, player, world, kind)) {
              spawned = true;
              break;
            }
          }
        }
        if (!spawned) break;
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
  // Front actors move first, freeing space for followers. Vector order/IDs
  // remain stable for C's animation and instance snapshots.
  std::vector<std::size_t> order(enemies_.size());
  for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
  std::stable_sort(order.begin(), order.end(),
                   [&](std::size_t a, std::size_t b) {
                     return distance(enemies_[a].position, player) <
                            distance(enemies_[b].position, player);
                   });
  constexpr int width = world::ArenaWorld::kWidth,
                depth = world::ArenaWorld::kDepth;
  for (const auto index : order) {
    auto& e = enemies_[index];
    const auto profile = enemyProfile(e.kind);
    const auto worldClear = [&](glm::vec3 from, glm::vec3 to) {
      from.y = to.y = profile.centreY;
      if (!world.canOccupy(from, profile.halfExtents) ||
          !world.canOccupy(to, profile.halfExtents))
        return false;
      const auto low = glm::floor(glm::min(from, to) - profile.halfExtents);
      const auto high = glm::floor(glm::max(from, to) + profile.halfExtents);
      // Continuous slab test against expanded voxels, with the same edge
      // tolerance as canOccupy. Tangent contact is allowed; penetration is not.
      // Sampling missed tiny corner clips and repeatedly selected blocked
      // paths.
      for (int y = static_cast<int>(low.y); y <= static_cast<int>(high.y); ++y)
        for (int z = static_cast<int>(low.z); z <= static_cast<int>(high.z);
             ++z)
          for (int x = static_cast<int>(low.x); x <= static_cast<int>(high.x);
               ++x) {
            if (!world.isSolid(x, y, z)) continue;
            const auto lo =
                glm::vec3(x, y, z) - profile.halfExtents + glm::vec3(.0001F);
            const auto hi = glm::vec3(x + 1, y + 1, z + 1) +
                            profile.halfExtents - glm::vec3(.0001F);
            float enter = 0, leave = 1;
            bool intersects = true;
            for (int axis = 0; axis < 3; ++axis) {
              const float d = to[axis] - from[axis];
              if (std::abs(d) < 1e-8F) {
                if (from[axis] <= lo[axis] || from[axis] >= hi[axis]) {
                  intersects = false;
                  break;
                }
              } else {
                float a = (lo[axis] - from[axis]) / d,
                      b = (hi[axis] - from[axis]) / d;
                if (a > b) std::swap(a, b);
                enter = std::max(enter, a);
                leave = std::min(leave, b);
                if (enter >= leave) {
                  intersects = false;
                  break;
                }
              }
            }
            if (intersects) return false;
          }
      return true;
    };
    const auto actorClear = [&](const glm::vec3& point) {
      for (const auto& other : enemies_)
        if (other.id != e.id &&
            overlaps(point, profile.halfExtents, other.position,
                     enemyProfile(other.kind).halfExtents))
          return false;
      return true;
    };
    const auto actorRouteClear = [&](glm::vec3 from, glm::vec3 to) {
      from.y = to.y = 0;
      for (const auto& other : enemies_)
        if (other.id != e.id) {
          auto centre = other.position;
          centre.y = 0;
          auto extents =
              profile.halfExtents + enemyProfile(other.kind).halfExtents;
          extents.x -= .00001F;
          extents.z -= .00001F;
          extents.y = 1;
          if (intersection(from, to, centre - extents, centre + extents) <= 1)
            return false;
        }
      return true;
    };
    // Stop outside the combined player/body footprint, rather than demanding
    // that a large golem fit at the player's centre next to a wall.
    const auto approach = [&](glm::vec3 from) {
      auto direction = from - player;
      direction.y = 0;
      const float length = glm::length(direction);
      if (length < .001F) return from;
      direction /= length;
      const auto combined = profile.halfExtents + playerHalfExtents();
      const float x = std::abs(direction.x) > .0001F
                          ? combined.x / std::abs(direction.x)
                          : 10000.F;
      const float z = std::abs(direction.z) > .0001F
                          ? combined.z / std::abs(direction.z)
                          : 10000.F;
      const float stop = std::min(x, z) + .015F;
      auto goal = player + direction * stop;
      goal.y = profile.centreY;
      return length > stop ? goal : from;
    };
    const auto previousPosition = e.position;
    if (e.waypoint && (!worldClear(e.position, *e.waypoint) ||
                       !actorRouteClear(e.position, *e.waypoint)))
      e.waypoint.reset();
    const auto direct = approach(e.position);
    if (worldClear(e.position, direct) && actorRouteClear(e.position, direct))
      e.waypoint = direct;
    if (e.waypoint && distance(e.position, *e.waypoint) < .02F)
      e.waypoint.reset();
    if (!e.waypoint) {
      const int x = static_cast<int>(std::floor(e.position.x)),
                z = static_cast<int>(std::floor(e.position.z));
      if (x < 0 || x >= width || z < 0 || z >= depth) continue;
      const int start = z * width + x;
      std::array<int, width * depth> parent;
      parent.fill(-1);
      std::queue<int> pending;
      pending.push(start);
      parent[start] = start;
      int destination = -1, fallback = start;
      float closest = distance(e.position, player);
      constexpr int steps[][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
      while (!pending.empty()) {
        const int cell = pending.front();
        pending.pop();
        const glm::vec3 centre =
            cell == start ? e.position
                          : glm::vec3(cell % width + .5F, profile.centreY,
                                      cell / width + .5F);
        const auto goal = approach(centre);
        if (distance(centre, player) < 1.8F && worldClear(centre, goal) &&
            actorRouteClear(centre, goal)) {
          destination = cell;
          break;
        }
        if (distance(centre, player) < closest - .1F) {
          closest = distance(centre, player);
          fallback = cell;
        }
        for (int offset = 0; offset < 4; ++offset) {
          const auto& step = steps[(offset + e.id) % 4];
          const int nx = cell % width + step[0], nz = cell / width + step[1];
          if (nx < 0 || nx >= width || nz < 0 || nz >= depth) continue;
          const int next = nz * width + nx;
          const glm::vec3 point(nx + .5F, profile.centreY, nz + .5F);
          if (parent[next] >= 0 ||
              !world.canOccupy(point, profile.halfExtents) ||
              !actorClear(point) || !actorRouteClear(centre, point))
            continue;
          if (cell == start && !worldClear(centre, point)) continue;
          parent[next] = cell;
          pending.push(next);
        }
      }
      if (destination < 0 && fallback != start) destination = fallback;
      if (destination == start)
        e.waypoint = approach(e.position);
      else if (destination >= 0) {
        int next = destination;
        while (parent[next] != start) next = parent[next];
        e.waypoint =
            glm::vec3(next % width + .5F, profile.centreY, next / width + .5F);
      }
    }
    // An off-centre body beside a corner may not connect directly to any
    // neighbouring centre. Connect through its own free cell before retrying.
    const auto anchor =
        glm::vec3(std::floor(e.position.x) + .5F, profile.centreY,
                  std::floor(e.position.z) + .5F);
    if (!e.waypoint && distance(e.position, anchor) > .02F &&
        worldClear(e.position, anchor) && actorRouteClear(e.position, anchor))
      e.waypoint = anchor;
    if (e.waypoint) {
      auto delta = *e.waypoint - e.position;
      delta.y = 0;
      const float length = glm::length(delta);
      if (length > .001F) {
        const float terrain =
            world.isWaterSurface(static_cast<int>(std::floor(e.position.x)),
                                 static_cast<int>(std::floor(e.position.z)))
                ? world::ArenaWorld::kWaterMovementSpeedMultiplier
                : 1.F;
        const float travel = std::min(length, profile.speed * terrain * dt);
        const int count =
            std::max(1, static_cast<int>(std::ceil(travel / .06F)));
        const auto step = delta / length * (travel / count);
        for (int i = 0; i < count; ++i) {
          const auto next = e.position + step;
          if (!world.canOccupy(next, profile.halfExtents) ||
              !actorClear(next) ||
              overlaps(next, profile.halfExtents, player,
                       playerHalfExtents())) {
            e.waypoint.reset();
            break;
          }
          e.position = next;
        }
      }
    }
    if (distance(previousPosition, e.position) < .0001F &&
        distance(e.position, player) > 1.1F) {
      e.blockedSeconds += dt;
      if (e.blockedSeconds > .35F) {
        e.waypoint.reset();
        if (distance(e.position, anchor) > .02F &&
            worldClear(e.position, anchor) &&
            actorRouteClear(e.position, anchor))
          e.waypoint = anchor;
        e.blockedSeconds = 0;
      }
    } else
      e.blockedSeconds = 0;
    const auto combined = profile.halfExtents + playerHalfExtents();
    const auto gap =
        glm::max(glm::abs(e.position - player) - combined, glm::vec3(0));
    if (glm::length(glm::vec2(gap.x, gap.z)) < .25F && hurtClock_ <= 0) {
      // Tiny LOS probe prevents melee through the corner of a solid block.
      bool visible = true;
      for (int i = 1; i <= 8; ++i)
        if (!world.canOccupy(e.position + (player - e.position) * (i / 8.F),
                             glm::vec3(.015F)))
          visible = false;
      if (visible) {
        stats_.health = std::max(0, stats_.health - profile.contactDamage);
        hurtClock_ = 1;
        auto direction = player - e.position;
        direction.y = 0;
        direction = glm::length(direction) > .0001F ? glm::normalize(direction)
                                                    : glm::vec3(0, 0, 1);
        attacks_.push_back({e.id, e.position, direction});
      }
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
            b.position, end,
            e.position - enemyProfile(e.kind).halfExtents - padding,
            e.position + enemyProfile(e.kind).halfExtents + padding);
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
        if (!victimAlreadyHit || evolvedLaserAfterBounce) {
          damageEnemy(*victim, b.damage, b.position, b.weapon);
          impacts_.push_back(
              {b.position, glm::normalize(b.velocity), b.weapon, b.evolved});
        }
        if (!victimAlreadyHit) b.hitEnemies.push_back(victim->id);
        if (b.weapon == Weapon::Bullet ||
            (b.weapon == Weapon::Laser && !b.piercesEnemies))
          consumed = true;
      } else if (b.weapon == Weapon::Laser &&
                 b.reflections < b.maxReflections &&
                 glm::length(wallNormal) > 0) {
        impacts_.push_back(
            {b.position, glm::normalize(b.velocity), b.weapon, b.evolved});
        for (int axis = 0; axis < 3; ++axis)
          if (wallNormal[axis] != 0) b.velocity[axis] = -b.velocity[axis];
        b.position += wallNormal * 0.002F;
        ++b.reflections;
        if (b.evolved) b.piercesEnemies = false;
      } else {
        impacts_.push_back(
            {b.position, glm::normalize(b.velocity), b.weapon, b.evolved});
        consumed = true;
      }
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
    orbs_.push_back(
        {enemy.position, enemyProfile(enemy.kind).experience, enemy.kind});
  }
}
void Gameplay::explode(const glm::vec3& position, int damage, float blastRadius,
                       bool firebomb) {
  impacts_.push_back(
      {position, {0, 1, 0}, Weapon::Bomb, firebomb, blastRadius});
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
WeaponProfile weaponProfile(Weapon weapon) { return weaponProfile(weapon, 0); }
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

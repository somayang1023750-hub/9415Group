#include "core/gameplay/Gameplay.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/geometric.hpp>
#include <limits>
#include <queue>

#include "world/ArenaWorld.h"

namespace craft::gameplay {
namespace {
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
  bullets_.clear();
  orbs_.clear();
  hits_.clear();
  shots_.clear();
  choices_.clear();
  nextId_ = 1;
  random_.seed(seed_);
  spawnClock_ = shotClock_ = hurtClock_ = 0;
}
bool Gameplay::spawnEnemy(const glm::vec3& p, const glm::vec3& player,
                          const world::ArenaWorld& world) {
  if (enemies_.size() >= 24 || distance(p, player) < 4 ||
      !world.canOccupy(p, enemyHalfExtents()))
    return false;
  for (const auto& e : enemies_)
    if (overlaps(p, e.position, 0.72F)) return false;
  enemies_.push_back({nextId_++, p});
  return true;
}
void Gameplay::offerUpgrade() {
  if (stats_.experience < experienceToNextLevel()) return;
  choices_ = {Weapon::Balance, Weapon::Laser, Weapon::Bomb};
  std::shuffle(choices_.begin(), choices_.end(), random_);
  choices_.resize(2);
  stats_.phase = Phase::ChoosingUpgrade;
}
bool Gameplay::chooseUpgrade(std::size_t choice) {
  if (stats_.phase != Phase::ChoosingUpgrade || choice >= choices_.size())
    return false;
  stats_.weapon = choices_[choice];
  stats_.experience -= experienceToNextLevel();
  ++stats_.level;
  choices_.clear();
  stats_.phase = Phase::Playing;
  offerUpgrade();
  return true;
}
void Gameplay::update(float dt, const glm::vec3& player,
                      const world::ArenaWorld& world,
                      std::optional<glm::vec3> target) {
  hits_.clear();
  shots_.clear();
  if (!std::isfinite(dt) || dt <= 0 || stats_.phase != Phase::Playing) return;
  dt = std::min(dt, 0.1F);
  for (auto& blast : explosions_) blast.age += dt;
  explosions_.erase(
      std::remove_if(explosions_.begin(), explosions_.end(),
                     [](const Explosion& blast) { return blast.age >= 0.4F; }),
      explosions_.end());
  stats_.remainingSeconds = std::max(0.F, stats_.remainingSeconds - dt);
  shotClock_ = std::max(0.F, shotClock_ - dt);
  hurtClock_ = std::max(0.F, hurtClock_ - dt);
  spawnClock_ -= dt;
  if (spawnClock_ <= 0) {
    const auto& candidates = world.enemySpawnCandidates();
    if (!candidates.empty()) {
      const std::size_t start = random_() % candidates.size();
      for (std::size_t i = 0; i < candidates.size(); ++i)
        if (spawnEnemy(candidates[(start + i) % candidates.size()], player,
                       world))
          break;
    }
    spawnClock_ = 2;
  }
  if (target && shotClock_ <= 0 && std::isfinite(target->x) &&
      std::isfinite(target->z)) {
    glm::vec3 direction{target->x - player.x, 0, target->z - player.z};
    if (glm::length(direction) > 0.001F) {
      const auto profile = weaponProfile(stats_.weapon);
      bullets_.push_back({player,
                          glm::normalize(direction) * profile.bulletSpeed, 3.F,
                          profile.baseDamage, stats_.weapon});
      shots_.push_back({player, glm::normalize(direction), stats_.weapon});
      shotClock_ = profile.shotCooldown;
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
        if (other.id != e.id && overlaps(point, other.position, 0.70F))
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
        const glm::vec3 centre(cell % width + 0.5F, 1.45F, cell / width + 0.5F);
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
          glm::vec3 point(nx + 0.5F, 1.45F, nz + 0.5F);
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
          const glm::vec3 centre(x + 0.5F, 1.45F, z + 0.5F);
          const glm::vec3 goal(next % width + 0.5F, 1.45F, next / width + 0.5F);
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
        const float travel = std::min(length, enemySpeed() * dt);
        const int count =
            std::max(1, static_cast<int>(std::ceil(travel / 0.08F)));
        const glm::vec3 step = delta / length * (travel / count);
        for (int i = 0; i < count; ++i) {
          const auto next = e.position + step;
          if (!world.canOccupy(next, enemyHalfExtents()) ||
              overlaps(next, player, 0.77F) || !actorClear(next)) {
            e.waypoint.reset();
            break;
          }
          e.position = next;
        }
      }
    }
    if (distance(e.position, player) < 1.15F && hurtClock_ <= 0) {
      --stats_.health;
      hurtClock_ = 1;
    }
  }
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
      for (auto& e : enemies_)
        if (e.health > 0 && std::find(b.hitEnemies.begin(), b.hitEnemies.end(),
                                      e.id) == b.hitEnemies.end()) {
          const float t = intersection(
              b.position, end, e.position - enemyHalfExtents() - padding,
              e.position + enemyHalfExtents() + padding);
          if (t < closest) {
            closest = t;
            victim = &e;
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
        explode(b.position, b.damage);
        consumed = true;
      } else if (victim) {
        damageEnemy(*victim, b.damage, b.position, b.weapon);
        b.hitEnemies.push_back(victim->id);
        if (b.weapon == Weapon::Balance) consumed = true;
      } else if (b.weapon == Weapon::Laser && b.reflections == 0 &&
                 glm::length(wallNormal) > 0) {
        for (int axis = 0; axis < 3; ++axis)
          if (wallNormal[axis] != 0) b.velocity[axis] = -b.velocity[axis];
        b.position += wallNormal * 0.002F;
        ++b.reflections;
      } else
        consumed = true;
    }
    if (!consumed && b.life <= 0 && b.weapon == Weapon::Bomb)
      explode(b.position, b.damage);
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
void Gameplay::explode(const glm::vec3& position, int damage) {
  explosions_.push_back({position, blastRadius()});
  // No falloff: every enemy centre inside the radius receives full damage,
  // including enemies behind cover. The player is not damaged by own bombs.
  for (auto& enemy : enemies_)
    if (distance(position, enemy.position) <= blastRadius())
      damageEnemy(enemy, damage, enemy.position, Weapon::Bomb);
}
WeaponProfile weaponProfile(Weapon weapon) {
  switch (weapon) {
    case Weapon::Laser:
      return {1, 0.12F, 40.F};
    case Weapon::Bomb:
      return {4, 1.2F, 8.F};
    default:
      return {2, 0.30F, 24.F};
  }
}
const char* weaponName(Weapon weapon) {
  switch (weapon) {
    case Weapon::Laser:
      return "Laser";
    case Weapon::Bomb:
      return "Bomb";
    default:
      return "Balance";
  }
}
const char* weaponDescription(Weapon weapon) {
  switch (weapon) {
    case Weapon::Laser:
      return "Pierces enemies. Reflects off one wall, stops at the next. Four "
             "hits per enemy.";
    case Weapon::Bomb:
      return "Explodes on enemies or walls. Full damage within 3.5 blocks. One "
             "hit per enemy.";
    default:
      return "Standard bullet. Stops on the first enemy or wall. Two hits per "
             "enemy.";
  }
}
}  // namespace craft::gameplay

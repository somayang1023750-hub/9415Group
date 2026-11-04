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
float intersection(glm::vec3 from, glm::vec3 to, glm::vec3 lo, glm::vec3 hi) {
  float enter = 0, leave = 1;
  for (int axis = 0; axis < 3; ++axis) {
    const float d = to[axis] - from[axis];
    if (std::abs(d) < 1e-7F) {
      if (from[axis] < lo[axis] || from[axis] > hi[axis]) return 2;
    } else {
      float a = (lo[axis] - from[axis]) / d, b = (hi[axis] - from[axis]) / d;
      if (a > b) std::swap(a, b);
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
  choices_ = {Weapon::Balanced, Weapon::Rapid, Weapon::Heavy};
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
      bullets_.push_back(
          {player, glm::normalize(direction) * 24.F, 2.F,
           (stats_.weapon == Weapon::Heavy ? 2 : 1) + stats_.level - 1,
           stats_.weapon});
      shots_.push_back({player, glm::normalize(direction), stats_.weapon});
      shotClock_ = stats_.weapon == Weapon::Rapid   ? 0.12F
                   : stats_.weapon == Weapon::Heavy ? 0.5F
                                                    : 0.25F;
    }
  }
  // B: rebuild the flow field from live blocks so setBlock/layout edits are
  // honoured. Four-neighbour routes prevent diagonal corner cutting.
  constexpr int width = world::ArenaWorld::kWidth,
                depth = world::ArenaWorld::kDepth;
  std::array<int, width * depth> costs;
  costs.fill(-1);
  std::queue<int> pending;
  const int px = static_cast<int>(std::floor(player.x));
  const int pz = static_cast<int>(std::floor(player.z));
  if (px >= 0 && px < width && pz >= 0 && pz < depth) {
    costs[pz * width + px] = 0;
    pending.push(pz * width + px);
  }
  constexpr int steps[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  while (!pending.empty()) {
    const int cell = pending.front();
    pending.pop();
    for (const auto& s : steps) {
      int x = cell % width + s[0], z = cell / width + s[1];
      if (x < 0 || x >= width || z < 0 || z >= depth) continue;
      const int n = z * width + x;
      if (costs[n] < 0 &&
          world.canOccupy({x + 0.5F, 1.45F, z + 0.5F}, enemyHalfExtents())) {
        costs[n] = costs[cell] + 1;
        pending.push(n);
      }
    }
  }
  for (auto& e : enemies_) {
    int x = static_cast<int>(std::floor(e.position.x));
    int z = static_cast<int>(std::floor(e.position.z));
    glm::vec3 goal = player;
    if (x < 0 || x >= width || z < 0 || z >= depth || costs[z * width + x] < 0)
      continue;
    int best = costs[z * width + x];
    if (best > 0) {
      goal = e.position;
      for (const auto& s : steps) {
        int nx = x + s[0], nz = z + s[1];
        if (nx >= 0 && nx < width && nz >= 0 && nz < depth &&
            costs[nz * width + nx] >= 0 && costs[nz * width + nx] < best) {
          best = costs[nz * width + nx];
          goal = {nx + 0.5F, 1.45F, nz + 0.5F};
        }
      }
      // Align to the current cell's centre before turning at a corner.
      if (std::abs(goal.x - e.position.x) > 0.01F &&
          std::abs(goal.z - e.position.z) > 0.01F)
        goal = {x + 0.5F, 1.45F, z + 0.5F};
    }
    glm::vec3 delta = goal - e.position;
    delta.y = 0;
    float length = glm::length(delta);
    if (length > 0.001F) {
      delta *= std::min(length, 1.8F * dt) / length;
      for (int axis : {0, 2}) {
        glm::vec3 next = e.position;
        next[axis] += delta[axis];
        bool clear = world.canOccupy(next, enemyHalfExtents()) &&
                     !overlaps(next, player, 0.77F);
        for (const auto& other : enemies_)
          if (other.id != e.id && overlaps(next, other.position, 0.70F))
            clear = false;
        if (clear) e.position = next;
      }
    }
    if (distance(e.position, player) < 1.15F && hurtClock_ <= 0) {
      --stats_.health;
      hurtClock_ = 1;
    }
  }
  for (auto& b : bullets_) {
    const auto end = b.position + b.velocity * dt;
    float closest = 2;
    Enemy* victim = nullptr;
    const glm::vec3 padding(radius);
    const auto low = glm::floor(glm::min(b.position, end) - padding);
    const auto high = glm::floor(glm::max(b.position, end) + padding);
    for (int z = static_cast<int>(low.z); z <= static_cast<int>(high.z); ++z)
      for (int y = static_cast<int>(low.y); y <= static_cast<int>(high.y); ++y)
        for (int x = static_cast<int>(low.x); x <= static_cast<int>(high.x);
             ++x)
          if (world.isSolid(x, y, z))
            closest = std::min(
                closest,
                intersection(b.position, end, glm::vec3(x, y, z) - padding,
                             glm::vec3(x + 1, y + 1, z + 1) + padding));
    for (auto& e : enemies_)
      if (e.health > 0) {
        float t = intersection(b.position, end,
                               e.position - enemyHalfExtents() - padding,
                               e.position + enemyHalfExtents() + padding);
        if (t < closest) {
          closest = t;
          victim = &e;
        }
      }
    if (closest <= 1) {
      b.life = 0;
      if (victim) {
        victim->health -= b.damage;
        hits_.push_back({b.position + (end - b.position) * closest, victim->id,
                         victim->health <= 0, b.weapon});
        if (victim->health <= 0) {
          ++stats_.kills;
          orbs_.push_back({victim->position});
        }
      }
    } else {
      b.position = end;
      b.life -= dt;
    }
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
const char* weaponName(Weapon weapon) {
  switch (weapon) {
    case Weapon::Rapid:
      return "Rapid (short cooldown)";
    case Weapon::Heavy:
      return "Heavy (slow, high damage)";
    default:
      return "Balanced";
  }
}
}  // namespace craft::gameplay

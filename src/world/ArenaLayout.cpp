#include "world/ArenaLayout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <stdexcept>
#include <vector>

#include "world/ArenaWorld.h"

namespace craft::world {
namespace {

constexpr int kNoiseTextureSize = 128;
constexpr int kObstacleTargetCount = 16;
constexpr int kWaterRegionArea = 112;
constexpr std::uint32_t kArenaNoiseSeed = 9415;
using NoiseTexture =
    std::array<float, kNoiseTextureSize * kNoiseTextureSize>;

struct NoiseLayer {
  int size;
  std::vector<float> values;
};

struct ObstacleCandidate {
  ArenaObstacle obstacle;
  float score;
};

struct WaterCandidate {
  int x;
  int z;
  float score;
};

std::uint32_t cellNoise(std::uint32_t seed, int x, int z) {
  std::uint32_t value = seed ^ (static_cast<std::uint32_t>(x) * 0x9E3779B9U) ^
                        (static_cast<std::uint32_t>(z) * 0x85EBCA6BU);
  value ^= value >> 16U;
  value *= 0x7FEB352DU;
  value ^= value >> 15U;
  value *= 0x846CA68BU;
  return value ^ (value >> 16U);
}

std::vector<ArenaWaterCell> makeWaterCells(std::uint32_t seed) {
  std::vector<WaterCandidate> candidates;
  candidates.reserve(15 * 15);
  for (int z = 1; z <= 15; ++z) {
    for (int x = 1; x <= 15; ++x) {
      const float dx = static_cast<float>(x) - 6.0F;
      const float dz = static_cast<float>(z) - 6.0F;
      const float randomEdge =
          (static_cast<float>(cellNoise(seed, x, z) & 0xFFFFU) / 65535.0F -
           0.5F) *
          1.2F;
      candidates.push_back({x, z, std::sqrt(dx * dx + dz * dz) + randomEdge});
    }
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const WaterCandidate& left, const WaterCandidate& right) {
              if (left.score != right.score) return left.score < right.score;
              if (left.z != right.z) return left.z < right.z;
              return left.x < right.x;
            });

  std::vector<ArenaWaterCell> cells;
  cells.reserve(2 * kWaterRegionArea);
  for (int i = 0; i < kWaterRegionArea; ++i) {
    const auto& candidate = candidates[static_cast<std::size_t>(i)];
    cells.push_back({candidate.x, candidate.z});
    // Mirror the first patch into the opposite corner to keep both areas
    // exactly equal while retaining the same irregular silhouette.
    cells.push_back({ArenaWorld::kWidth - 1 - candidate.x,
                     ArenaWorld::kDepth - 1 - candidate.z});
  }
  return cells;
}

float smooth(float value) {
  return value * value * (3.0F - 2.0F * value);
}

float interpolate(float from, float to, float amount) {
  return from + (to - from) * amount;
}

NoiseLayer makeNoiseLayer(int size, std::mt19937& random) {
  std::uniform_real_distribution<float> value(0.0F, 1.0F);
  NoiseLayer layer{size, std::vector<float>(size * size)};
  for (float& texel : layer.values) texel = value(random);
  return layer;
}

float sampleLayer(const NoiseLayer& layer, float u, float v) {
  const float px = std::clamp(u, 0.0F, 1.0F) * (layer.size - 1);
  const float py = std::clamp(v, 0.0F, 1.0F) * (layer.size - 1);
  const int x0 = static_cast<int>(std::floor(px));
  const int y0 = static_cast<int>(std::floor(py));
  const int x1 = std::min(x0 + 1, layer.size - 1);
  const int y1 = std::min(y0 + 1, layer.size - 1);
  const float tx = smooth(px - x0);
  const float ty = smooth(py - y0);
  const auto at = [&](int x, int y) {
    return layer.values[static_cast<std::size_t>(y) * layer.size + x];
  };
  const float top = interpolate(at(x0, y0), at(x1, y0), tx);
  const float bottom = interpolate(at(x0, y1), at(x1, y1), tx);
  return interpolate(top, bottom, ty);
}

NoiseTexture makeNoiseTexture(std::mt19937& random) {
  // The 128 x 128 texture covers the full 32 x 32 arena at four texels per
  // world unit. Its 4/8/16-cell layers add broad regions and detail at roughly
  // 8/4/2 world-unit scales, which fits the 2-3 block obstacle footprints.
  const std::array<NoiseLayer, 3> layers{
      makeNoiseLayer(5, random), makeNoiseLayer(9, random),
      makeNoiseLayer(17, random)};
  NoiseTexture texture{};
  for (int y = 0; y < kNoiseTextureSize; ++y) {
    for (int x = 0; x < kNoiseTextureSize; ++x) {
      const float u = static_cast<float>(x) / (kNoiseTextureSize - 1);
      const float v = static_cast<float>(y) / (kNoiseTextureSize - 1);
      const float broad = sampleLayer(layers[0], u, v);
      const float medium = sampleLayer(layers[1], u, v);
      const float detail = sampleLayer(layers[2], u, v);
      texture[static_cast<std::size_t>(y) * kNoiseTextureSize + x] =
          0.52F * broad + 0.31F * medium + 0.17F * detail;
    }
  }
  return texture;
}

float sampleTexture(const NoiseTexture& texture, float u, float v) {
  const float px = std::clamp(u, 0.0F, 1.0F) * (kNoiseTextureSize - 1);
  const float py = std::clamp(v, 0.0F, 1.0F) * (kNoiseTextureSize - 1);
  const int x0 = static_cast<int>(std::floor(px));
  const int y0 = static_cast<int>(std::floor(py));
  const int x1 = std::min(x0 + 1, kNoiseTextureSize - 1);
  const int y1 = std::min(y0 + 1, kNoiseTextureSize - 1);
  const float tx = px - x0;
  const float ty = py - y0;
  const auto at = [&](int x, int y) {
    return texture[static_cast<std::size_t>(y) * kNoiseTextureSize + x];
  };
  const float top = interpolate(at(x0, y0), at(x1, y0), tx);
  const float bottom = interpolate(at(x0, y1), at(x1, y1), tx);
  return interpolate(top, bottom, ty);
}

bool separatedFrom(const ArenaObstacle& candidate,
                   const ArenaObstacle& placed) {
  // Leave at least one walkable block between cover groups.
  return candidate.x >= placed.x + placed.width + 1 ||
         placed.x >= candidate.x + candidate.width + 1 ||
         candidate.z >= placed.z + placed.depth + 1 ||
         placed.z >= candidate.z + candidate.depth + 1;
}

bool clearOfSpawn(const ArenaObstacle& obstacle, const glm::vec3& spawn) {
  constexpr float kSpawnClearance = 1.0F;
  const float right = static_cast<float>(obstacle.x + obstacle.width);
  const float back = static_cast<float>(obstacle.z + obstacle.depth);
  return right <= spawn.x - kSpawnClearance ||
         static_cast<float>(obstacle.x) >= spawn.x + kSpawnClearance ||
         back <= spawn.z - kSpawnClearance ||
         static_cast<float>(obstacle.z) >= spawn.z + kSpawnClearance;
}

}  // namespace

ArenaLayout makeMilestoneOneLayout() {
  return makeMilestoneOneLayout(kArenaNoiseSeed);
}

ArenaLayout makeMilestoneOneLayout(std::uint32_t seed) {
  ArenaLayout layout;
  layout.waterCells = makeWaterCells(seed);
  layout.enemySpawnCandidates = {
      {16.0F, 1.45F, 2.5F},  {29.5F, 1.45F, 16.0F},
      {16.0F, 1.45F, 29.5F}, {2.5F, 1.45F, 16.0F},
      {3.5F, 1.45F, 3.5F},   {28.5F, 1.45F, 3.5F},
      {3.5F, 1.45F, 28.5F},  {28.5F, 1.45F, 28.5F},
  };

  std::mt19937 random(seed);
  const NoiseTexture noiseTexture = makeNoiseTexture(random);
  std::uniform_int_distribution<int> dimension(2, 3);
  std::uniform_int_distribution<int> height(1, 2);
  std::uniform_real_distribution<float> placementJitter(0.0F, 0.18F);
  std::vector<ObstacleCandidate> candidates;
  candidates.reserve((ArenaWorld::kWidth - 2) * (ArenaWorld::kDepth - 2));
  std::array<bool, static_cast<std::size_t>(ArenaWorld::kWidth) *
                       ArenaWorld::kDepth>
      waterMask{};
  for (const auto& waterCell : layout.waterCells) {
    waterMask[static_cast<std::size_t>(waterCell.z) * ArenaWorld::kWidth +
              waterCell.x] = true;
  }

  for (int z = 1; z < ArenaWorld::kDepth - 1; ++z) {
    for (int x = 1; x < ArenaWorld::kWidth - 1; ++x) {
      const ArenaObstacle obstacle{x, z, dimension(random), dimension(random),
                                   height(random)};
      if (x + obstacle.width >= ArenaWorld::kWidth - 1 ||
          z + obstacle.depth >= ArenaWorld::kDepth - 1) {
        continue;
      }

      // Preserve the player's initial movement area and all enemy entries.
      const bool clearOfPlayerStart =
          obstacle.x + obstacle.width <= 13 || obstacle.x >= 19 ||
          obstacle.z + obstacle.depth <= 13 || obstacle.z >= 19;
      if (!clearOfPlayerStart) continue;
      bool clearOfAllSpawns = true;
      for (const auto& spawn : layout.enemySpawnCandidates) {
        if (!clearOfSpawn(obstacle, spawn)) {
          clearOfAllSpawns = false;
          break;
        }
      }
      if (!clearOfAllSpawns) continue;

      bool clearOfWater = true;
      for (int dz = 0; dz < obstacle.depth && clearOfWater; ++dz) {
        for (int dx = 0; dx < obstacle.width; ++dx) {
          const std::size_t column =
              static_cast<std::size_t>(z + dz) * ArenaWorld::kWidth + x + dx;
          if (waterMask[column]) {
            clearOfWater = false;
            break;
          }
        }
      }
      if (!clearOfWater) continue;

      const float centerU =
          (static_cast<float>(x) + obstacle.width * 0.5F) /
          ArenaWorld::kWidth;
      const float centerV =
          (static_cast<float>(z) + obstacle.depth * 0.5F) /
          ArenaWorld::kDepth;
      const float noise = sampleTexture(noiseTexture, centerU, centerV);
      candidates.push_back({obstacle, noise + placementJitter(random)});
    }
  }

  std::sort(candidates.begin(), candidates.end(),
            [](const ObstacleCandidate& left,
               const ObstacleCandidate& right) {
              return left.score > right.score;
            });

  std::size_t coveredArea = 0;
  for (const auto& candidate : candidates) {
    if (layout.obstacles.size() >= kObstacleTargetCount) break;
    const auto& obstacle = candidate.obstacle;
    const std::size_t footprint =
        static_cast<std::size_t>(obstacle.width) * obstacle.depth;
    if (footprint > ArenaWorld::kMaximumObstacleArea - coveredArea) continue;

    const bool separated = std::all_of(
        layout.obstacles.begin(), layout.obstacles.end(),
        [&](const ArenaObstacle& placed) {
          return separatedFrom(obstacle, placed);
        });
    if (!separated) continue;

    layout.obstacles.push_back(obstacle);
    coveredArea += footprint;
  }

  if (layout.obstacles.size() != kObstacleTargetCount) {
    throw std::logic_error("Noise-based arena generation could not place cover");
  }
  return layout;
}

ArenaLayout makeArenaLayout(ArenaLevel level) {
  if (level == ArenaLevel::Day) return makeMilestoneOneLayout();

  ArenaLayout layout;
  // Three broken rows of barricades. Offset openings force a turn through
  // the centre, while a continuous outer lane connects all eight entries.
  layout.obstacles = {
      {4, 7, 10, 2, 2},  {19, 7, 9, 2, 2},
      {8, 11, 2, 5, 1},  {23, 12, 2, 4, 1},
      {5, 17, 6, 2, 2},  {15, 17, 12, 2, 2},
      {13, 22, 2, 5, 1}, {19, 23, 8, 2, 2},
  };
  layout.playerSpawn = {8.5F, 1.45F, 24.5F};
  layout.enemySpawnCandidates = {
      {3.5F, 1.45F, 3.5F},   {16.5F, 1.45F, 2.5F},
      {28.5F, 1.45F, 3.5F},  {29.5F, 1.45F, 14.5F},
      {28.5F, 1.45F, 28.5F}, {17.5F, 1.45F, 29.5F},
      {3.5F, 1.45F, 28.5F},  {2.5F, 1.45F, 13.5F},
  };
  return layout;
}

}  // namespace craft::world

#pragma once

#include <glm/vec4.hpp>
#include <random>
#include <vector>

#include "core/gameplay/Gameplay.h"

namespace craft::gameplay {
struct ParticleVisual {
  glm::vec3 position;
  glm::vec3 halfExtents;
  glm::vec3 direction;
  glm::vec4 color;
};
struct EffectLight {
  glm::vec3 position;
  glm::vec3 color;
  float radius;
  float strength;
};
// Pure presentation. Independent RNG and bounded buffers never change damage,
// cooldowns, spawn selection or persistent gameplay state. Scene consumes each
// frame's impacts once, only when simulation advances.
class ParticleEffects {
 public:
  static constexpr std::size_t kParticleLimit = 2048;
  static constexpr std::size_t kVisualLimit = 4096;
  static constexpr std::size_t kLightLimit = 8;
  void reset();
  void update(float dt, const std::vector<ImpactEvent>& impacts,
              const std::vector<BurningArea>& burningAreas,
              const std::vector<Bullet>& bullets = {});
  [[nodiscard]] const std::vector<ParticleVisual>& visuals() const {
    return visuals_;
  }

  [[nodiscard]] const std::vector<EffectLight>& lights() const {
    return lights_;
  }

 private:
  struct Flash {
    EffectLight light;
    float age;
    float lifetime;
  };
  std::vector<Flash> flashes_;
  std::vector<EffectLight> lights_;
  struct Particle {
    ParticleVisual visual;
    glm::vec3 velocity;
    float age = 0;
    float lifetime;
    float gravity;
    bool electric = false;
  };
  float random(float lo, float hi);
  void emit(const ImpactEvent& hit);
  void add(ParticleVisual visual, glm::vec3 velocity, float life,
           float gravity = 0, bool electric = false);
  std::mt19937 random_{9415};
  std::vector<Particle> particles_;
  std::vector<ParticleVisual> visuals_;
};
}  // namespace craft::gameplay

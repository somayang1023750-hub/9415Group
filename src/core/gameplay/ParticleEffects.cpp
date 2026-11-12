#include "core/gameplay/ParticleEffects.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
namespace craft::gameplay {
namespace {
constexpr float pi = 3.14159265F;
}
float ParticleEffects::random(float lo, float hi) {
  return lo + (hi - lo) * static_cast<float>(random_() >> 8) / 16777216.F;
}
void ParticleEffects::reset() {
  flashes_.clear();
  lights_.clear();
  particles_.clear();
  visuals_.clear();
  groundFire_.clear();
  random_.seed(9415);
}
void ParticleEffects::add(ParticleVisual visual, glm::vec3 velocity, float life,
                          float gravity, bool electric) {
  if (particles_.size() < kParticleLimit)
    particles_.push_back({visual, velocity, 0, life, gravity, electric});
}
void ParticleEffects::emit(const ImpactEvent& hit) {
  if (flashes_.size() >= kLightLimit) flashes_.erase(flashes_.begin());
  const bool bomb = hit.weapon == Weapon::Bomb,
             laser = hit.weapon == Weapon::Laser;
  const glm::vec3 color = hit.evolved ? glm::vec3(1, .025F, .06F)
                          : laser     ? glm::vec3(.02F, .7F, 1)
                          : bomb      ? glm::vec3(1, .3F, .04F)
                                      : glm::vec3(1, .8F, .45F);
  flashes_.push_back({{hit.position + glm::vec3(0, .2F, 0), color,
                       bomb    ? std::min(4.F, hit.radius + 1.F)
                       : laser ? 1.6F
                               : 1.3F,
                       bomb    ? .24F
                       : laser ? .14F
                               : .10F},
                      0,
                      bomb ? .5F : .18F});
  if (hit.weapon == Weapon::Bullet) {
    for (int i = 0; i < 14; ++i) {
      const float a = random(0, 2 * pi), speed = random(1.4F, 3.8F);
      const auto size = glm::vec3(random(.017F, .035F));
      add({hit.position, size, {0, 1, 0}, {1.F, random(.6F, 1.F), .35F, 1}},
          {std::cos(a) * speed, random(.4F, 2.4F), std::sin(a) * speed},
          random(.16F, .28F), 12);
    }
  } else if (hit.weapon == Weapon::Laser) {
    const glm::vec4 color =
        hit.evolved ? glm::vec4(1, .04F, .12F, 1) : glm::vec4(.08F, .85F, 1, 1);
    for (int branch = 0; branch < 5; ++branch) {
      auto from = hit.position;
      const float a = random(0, 2 * pi);
      const glm::vec3 outward(std::cos(a), random(-.3F, .6F), std::sin(a));
      for (int segment = 0; segment < 5; ++segment) {
        auto to = from + outward * .13F +
                  glm::vec3(random(-.045F, .045F), random(-.09F, .09F),
                            random(-.045F, .045F));
        to.y = std::max(1.035F, to.y);
        const auto delta = to - from;
        add({(to + from) * .5F,
             {.009F, glm::length(delta) * .5F, .009F},
             glm::normalize(delta),
             color},
            outward * .25F, random(.12F, .22F), 0, true);
        from = to;
      }
    }
  } else {
    for (int i = 0; i < 64; ++i) {
      const float a = (i + random(-.35F, .35F)) * 2 * pi / 64;
      const float speed = std::max(.5F, hit.radius) * random(1.4F, 2.2F);
      const glm::vec4 color = hit.evolved
                                  ? glm::vec4(1, .06F, .09F, 1)
                                  : glm::vec4(1, random(.25F, .75F), .06F, 1);
      add({hit.position, glm::vec3(random(.025F, .055F)), {0, 1, 0}, color},
          {std::cos(a) * speed, random(.4F, 2.F), std::sin(a) * speed},
          random(.38F, .65F), 5);
    }
  }
}
void ParticleEffects::update(float dt, const std::vector<ImpactEvent>& impacts,
                             const std::vector<BurningArea>& areas) {
  if (!std::isfinite(dt) || dt <= 0) return;
  dt = std::min(dt, .1F);
  for (auto& flash : flashes_) flash.age += dt;
  flashes_.erase(
      std::remove_if(flashes_.begin(), flashes_.end(),
                     [](const Flash& f) { return f.age >= f.lifetime; }),
      flashes_.end());
  for (auto& p : particles_) {
    p.age += dt;
    p.velocity.y -= p.gravity * dt;
    p.visual.position += p.velocity * dt;
    if (p.visual.position.y < 1.025F) {
      p.visual.position.y = 1.025F;
      p.velocity.y = 0;
    }
  }
  particles_.erase(
      std::remove_if(particles_.begin(), particles_.end(),
                     [](const Particle& p) { return p.age >= p.lifetime; }),
      particles_.end());
  for (const auto& hit : impacts) {
    if (particles_.size() >= kParticleLimit) break;
    emit(hit);
  }
  visuals_.clear();
  visuals_.reserve(particles_.size());
  for (const auto& p : particles_) {
    auto v = p.visual;
    const float fade = 1 - p.age / p.lifetime;
    v.halfExtents *= .35F + .65F * fade;
    v.color.a *=
        fade * (p.electric ? .55F + .45F * std::abs(std::sin(p.age * 95)) : 1);
    visuals_.push_back(v);
  }
  // Preserve the pre-effects fire: ten .035 cubes, the original two colours,
  // four orbital radii and rising cycle. Only rotation changes (.8 -> 3.2
  // rad/s). Keep these lit like the original proxies; damage/lifetime stay in
  // Gameplay.
  groundFire_.clear();
  for (const auto& area : areas) {
    if (area.age >= 3 || area.radius <= 0) continue;
    for (int i = 0;
         i < 10 && visuals_.size() + groundFire_.size() < kVisualLimit; ++i) {
      const float angle = i * 2.39996323F + area.age * 3.2F;
      const float radialDistance = area.radius * (.22F + .16F * (i % 4));
      const float rise = .08F + std::fmod(area.age * 1.8F + i * .27F, .72F);
      const glm::vec3 pos(area.position.x + std::cos(angle) * radialDistance,
                          1.08F + rise,
                          area.position.z + std::sin(angle) * radialDistance);
      const glm::vec4 color = i % 2 == 0 ? glm::vec4(1, .18F, .025F, 1)
                                         : glm::vec4(1, .48F, .045F, 1);
      groundFire_.push_back({pos, glm::vec3(.035F), {0, 1, 0}, color});
    }
    if (visuals_.size() + groundFire_.size() >= kVisualLimit) break;
  }
  lights_.clear();
  for (const auto& area : areas) {
    if (lights_.size() >= kLightLimit) break;
    if (area.age < 3 && area.radius > 0)
      lights_.push_back({area.position + glm::vec3(0, .25F, 0),
                         {1, .16F, .025F},
                         std::min(4.F, area.radius + .4F),
                         .12F * std::min(1.F, (3 - area.age) / .35F)});
  }
  for (auto it = flashes_.rbegin();
       it != flashes_.rend() && lights_.size() < kLightLimit; ++it) {
    auto light = it->light;
    light.strength *= 1 - it->age / it->lifetime;
    lights_.push_back(light);
  }
}
}  // namespace craft::gameplay

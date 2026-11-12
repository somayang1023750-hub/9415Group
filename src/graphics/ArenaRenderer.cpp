#include "graphics/ArenaRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <glm/ext/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

#include "core/AssetPath.h"
#include "core/gameplay/ActorGeometry.h"
#include "graphics/OpenGL.h"
#include "graphics/Shader.h"
#include "world/ArenaWorld.h"

namespace craft::graphics {

namespace {

struct FaceDefinition {
  int neighborX;
  int neighborY;
  int neighborZ;
  std::array<glm::vec3, 4> corners;
};

const std::array<FaceDefinition, 6> kCubeFaces{{
    // +X
    {1,
     0,
     0,
     {{{1.0F, 0.0F, 0.0F},
       {1.0F, 1.0F, 0.0F},
       {1.0F, 1.0F, 1.0F},
       {1.0F, 0.0F, 1.0F}}}},
    // -X
    {-1,
     0,
     0,
     {{{0.0F, 0.0F, 1.0F},
       {0.0F, 1.0F, 1.0F},
       {0.0F, 1.0F, 0.0F},
       {0.0F, 0.0F, 0.0F}}}},
    // +Y
    {0,
     1,
     0,
     {{{0.0F, 1.0F, 0.0F},
       {0.0F, 1.0F, 1.0F},
       {1.0F, 1.0F, 1.0F},
       {1.0F, 1.0F, 0.0F}}}},
    // -Y
    {0,
     -1,
     0,
     {{{0.0F, 0.0F, 1.0F},
       {0.0F, 0.0F, 0.0F},
       {1.0F, 0.0F, 0.0F},
       {1.0F, 0.0F, 1.0F}}}},
    // +Z
    {0,
     0,
     1,
     {{{0.0F, 0.0F, 1.0F},
       {1.0F, 0.0F, 1.0F},
       {1.0F, 1.0F, 1.0F},
       {0.0F, 1.0F, 1.0F}}}},
    // -Z
    {0,
     0,
     -1,
     {{{1.0F, 0.0F, 0.0F},
       {0.0F, 0.0F, 0.0F},
       {0.0F, 1.0F, 0.0F},
       {1.0F, 1.0F, 0.0F}}}},
}};

struct UvRect {
  float x;
  float y;
  float width;
  float height;
};

// Face order matches kCubeFaces: +X, -X, +Y, -Y, +Z (front), -Z (back).
// Coordinates use the classic 64 x 64 Minecraft player-skin layout.
struct SkinUvLayout {
  std::array<UvRect, 6> faces;
};

constexpr SkinUvLayout kHeadSkin{{{
    {16.0F, 8.0F, 8.0F, 8.0F},  // left
    {0.0F, 8.0F, 8.0F, 8.0F},   // right
    {8.0F, 0.0F, 8.0F, 8.0F},   // top
    {16.0F, 0.0F, 8.0F, 8.0F},  // bottom
    {8.0F, 8.0F, 8.0F, 8.0F},   // front
    {24.0F, 8.0F, 8.0F, 8.0F},  // back
}}};

constexpr SkinUvLayout kTorsoSkin{{{
    {28.0F, 20.0F, 4.0F, 12.0F},  // left
    {16.0F, 20.0F, 4.0F, 12.0F},  // right
    {20.0F, 16.0F, 8.0F, 4.0F},   // top
    {28.0F, 16.0F, 8.0F, 4.0F},   // bottom
    {20.0F, 20.0F, 8.0F, 12.0F},  // front
    {32.0F, 20.0F, 8.0F, 12.0F},  // back
}}};

constexpr SkinUvLayout kRightArmSkin{{{
    {48.0F, 20.0F, 4.0F, 12.0F},  // left
    {40.0F, 20.0F, 4.0F, 12.0F},  // right
    {44.0F, 16.0F, 4.0F, 4.0F},   // top
    {48.0F, 16.0F, 4.0F, 4.0F},   // bottom
    {44.0F, 20.0F, 4.0F, 12.0F},  // front
    {52.0F, 20.0F, 4.0F, 12.0F},  // back
}}};

constexpr SkinUvLayout kLeftArmSkin{{{
    {40.0F, 52.0F, 4.0F, 12.0F},  // left
    {32.0F, 52.0F, 4.0F, 12.0F},  // right
    {36.0F, 48.0F, 4.0F, 4.0F},   // top
    {40.0F, 48.0F, 4.0F, 4.0F},   // bottom
    {36.0F, 52.0F, 4.0F, 12.0F},  // front
    {44.0F, 52.0F, 4.0F, 12.0F},  // back
}}};

constexpr SkinUvLayout kRightLegSkin{{{
    {8.0F, 20.0F, 4.0F, 12.0F},   // left
    {0.0F, 20.0F, 4.0F, 12.0F},   // right
    {4.0F, 16.0F, 4.0F, 4.0F},    // top
    {8.0F, 16.0F, 4.0F, 4.0F},    // bottom
    {4.0F, 20.0F, 4.0F, 12.0F},   // front
    {12.0F, 20.0F, 4.0F, 12.0F},  // back
}}};

constexpr SkinUvLayout kLeftLegSkin{{{
    {24.0F, 52.0F, 4.0F, 12.0F},  // left
    {16.0F, 52.0F, 4.0F, 12.0F},  // right
    {20.0F, 48.0F, 4.0F, 4.0F},   // top
    {24.0F, 48.0F, 4.0F, 4.0F},   // bottom
    {20.0F, 52.0F, 4.0F, 12.0F},  // front
    {28.0F, 52.0F, 4.0F, 12.0F},  // back
}}};

constexpr std::array<SkinUvLayout, ArenaRenderer::kPlayerPartCount>
    kPlayerSkinLayouts{{kHeadSkin, kTorsoSkin, kRightArmSkin, kLeftArmSkin,
                        kRightLegSkin, kLeftLegSkin}};
// The standard 64x64 Minecraft zombie atlas uses the same base biped UV map.
constexpr std::array<SkinUvLayout, ArenaRenderer::kZombiePartCount>
    kZombieSkinLayouts{{kHeadSkin, kTorsoSkin, kRightArmSkin, kLeftArmSkin,
                        kRightLegSkin, kLeftLegSkin}};
constexpr float kSkinPixelSize = 0.05F;
constexpr float kPlayerFeetOffset = gameplay::kActorCentreY - 1.0F;
constexpr float kWalkSwingAmplitude = 0.55F;
constexpr float kZombieArmForwardAngle = -1.48F;
constexpr float kSandTextureTileWorldSize = 4.0F;

void appendVertex(std::vector<float>& vertices, const glm::vec3& position,
                  const glm::vec3& color) {
  vertices.insert(vertices.end(), {position.x, position.y, position.z, color.r,
                                   color.g, color.b});
}

void appendTexturedVertex(std::vector<float>& vertices,
                          const glm::vec3& position, const glm::vec3& color,
                          const glm::vec2& uv, const glm::vec3& normal) {
  vertices.insert(vertices.end(),
                  {position.x, position.y, position.z, color.r, color.g,
                   color.b, uv.x, uv.y, normal.x, normal.y, normal.z});
}

// Surface UVs increase with X/Z on horizontal faces, and across/down on
// upright faces. The shader uses those same axes for the tangent-space normals.
void appendMasonryQuad(std::vector<float>& vertices,
                       const std::array<glm::vec3, 4>& corners,
                       const glm::vec3& color, const glm::vec3& normal) {
  constexpr int indices[] = {0, 1, 2, 0, 2, 3};
  for (int index : indices) {
    const auto& p = corners[index];
    glm::vec2 uv{normal.x != 0 ? p.z : p.x, normal.y != 0 ? p.z : -p.y};
    appendTexturedVertex(vertices, p, color, uv * 0.5F, normal);
  }
}

void appendTerrainQuad(std::vector<float>& vertices, int x, int z,
                       const glm::vec3& color, float textureWorldSize) {
  constexpr int kTriangleCorners[] = {0, 1, 2, 0, 2, 3};
  const float x0 = static_cast<float>(x);
  const float z0 = static_cast<float>(z);
  const float x1 = x0 + 1.0F;
  const float z1 = z0 + 1.0F;
  const std::array<glm::vec3, 4> positions{{
      {x0, 1.0F, z0}, {x0, 1.0F, z1}, {x1, 1.0F, z1}, {x1, 1.0F, z0}}};
  const std::array<glm::vec2, 4> uvs{{
      {x0 / textureWorldSize, z0 / textureWorldSize},
      {x0 / textureWorldSize, z1 / textureWorldSize},
      {x1 / textureWorldSize, z1 / textureWorldSize},
      {x1 / textureWorldSize, z0 / textureWorldSize}}};
  const glm::vec3 kUp{0.0F, 1.0F, 0.0F};
  for (const int corner : kTriangleCorners) {
    appendTexturedVertex(vertices, positions[corner], color, uvs[corner], kUp);
  }
}

std::array<glm::vec2, 4> faceUvCoordinates(const UvRect& rect,
                                           std::size_t faceIndex,
                                           int textureWidth,
                                           int textureHeight) {
  const float u0 = rect.x / static_cast<float>(textureWidth);
  const float u1 = (rect.x + rect.width) / static_cast<float>(textureWidth);
  // Texture loads keep the PNG's first row at v = 0.
  const float v0 = rect.y / static_cast<float>(textureHeight);
  const float v1 = (rect.y + rect.height) / static_cast<float>(textureHeight);

  switch (faceIndex) {
    case 0:  // +X
    case 1:  // -X
      return {glm::vec2{u1, v1}, glm::vec2{u1, v0}, glm::vec2{u0, v0},
              glm::vec2{u0, v1}};
    case 2:  // +Y, with the front edge along z = 1
      return {glm::vec2{u0, v0}, glm::vec2{u0, v1}, glm::vec2{u1, v1},
              glm::vec2{u1, v0}};
    case 3:  // -Y
      return {glm::vec2{u0, v0}, glm::vec2{u0, v1}, glm::vec2{u1, v1},
              glm::vec2{u1, v0}};
    case 4:  // +Z front
    case 5:  // -Z back
    default:
      return {glm::vec2{u0, v1}, glm::vec2{u1, v1}, glm::vec2{u1, v0},
              glm::vec2{u0, v0}};
  }
}

void appendTexturedBox(std::vector<float>& vertices,
                       const SkinUvLayout& skinLayout, int textureWidth,
                       int textureHeight) {
  constexpr int kTriangleCorners[] = {0, 1, 2, 0, 2, 3};
  const glm::vec3 origin{-0.5F};

  for (std::size_t faceIndex = 0; faceIndex < kCubeFaces.size(); ++faceIndex) {
    const FaceDefinition& face = kCubeFaces[faceIndex];
    const auto uvs = faceUvCoordinates(skinLayout.faces[faceIndex], faceIndex,
                                       textureWidth, textureHeight);
    const glm::vec3 normal{static_cast<float>(face.neighborX),
                           static_cast<float>(face.neighborY),
                           static_cast<float>(face.neighborZ)};
    for (const int cornerIndex : kTriangleCorners) {
      const glm::vec3 position = origin + face.corners[cornerIndex];
      appendTexturedVertex(vertices, position, glm::vec3{1.0F},
                           uvs[cornerIndex], normal);
    }
  }
}

glm::vec3 blockColor(world::BlockType type, int x, int z) {
  switch (type) {
    case world::BlockType::Floor:
      return ((x + z) % 2 == 0) ? glm::vec3{0.36F, 0.43F, 0.38F}
                                : glm::vec3{0.31F, 0.38F, 0.34F};
    case world::BlockType::Boundary:
      return {0.35F, 0.42F, 0.52F};
    case world::BlockType::Obstacle:
      return {0.53F, 0.39F, 0.27F};
    case world::BlockType::Air:
      break;
  }
  return {0.0F, 0.0F, 0.0F};
}

void appendBlock(std::vector<float>& vertices,
                 const world::ArenaWorld& arenaWorld, int x, int y, int z,
                 world::BlockType type) {
  const glm::vec3 origin{static_cast<float>(x), static_cast<float>(y),
                         static_cast<float>(z)};
  const glm::vec3 baseColor = blockColor(type, x, z);

  for (const FaceDefinition& face : kCubeFaces) {
    if (face.neighborY < 0 && y == 0) {
      continue;
    }
    if (type == world::BlockType::Floor && y == 0 && face.neighborY > 0) {
      continue;
    }
    if (arenaWorld.blockAt(x + face.neighborX, y + face.neighborY,
                           z + face.neighborZ) != world::BlockType::Air) {
      continue;
    }

    std::array<glm::vec3, 4> worldCorners;
    for (std::size_t i = 0; i < face.corners.size(); ++i) {
      worldCorners[i] =
          origin + face.corners[i] * world::ArenaWorld::kBlockSize;
    }
    const glm::vec3 normal{static_cast<float>(face.neighborX),
                           static_cast<float>(face.neighborY),
                           static_cast<float>(face.neighborZ)};
    appendMasonryQuad(vertices, worldCorners, baseColor, normal);
  }
}

void configureVertexArray(unsigned int vertexArray, unsigned int vertexBuffer,
                          const std::vector<float>& vertices,
                          bool hasTextureCoordinates = false) {
  glBindVertexArray(vertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
               vertices.empty() ? nullptr : vertices.data(), GL_STATIC_DRAW);

  const GLsizei stride =
      static_cast<GLsizei>((hasTextureCoordinates ? 8 : 6) * sizeof(float));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                        reinterpret_cast<const void*>(3 * sizeof(float)));
  if (hasTextureCoordinates) {
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(6 * sizeof(float)));
  }
  glBindVertexArray(0);
}

void configureLitVertexArray(unsigned int vertexArray,
                             unsigned int vertexBuffer,
                             const std::vector<float>& vertices,
                             bool hasTextureCoordinates) {
  const int vertexFloatCount = hasTextureCoordinates ? 11 : 9;
  glBindVertexArray(vertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
               vertices.empty() ? nullptr : vertices.data(), GL_STATIC_DRAW);
  const GLsizei stride =
      static_cast<GLsizei>(vertexFloatCount * sizeof(float));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, stride,
      reinterpret_cast<const void*>(3 * sizeof(float)));
  if (hasTextureCoordinates) {
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void*>(6 * sizeof(float)));
  }
  glEnableVertexAttribArray(7);
  glVertexAttribPointer(
      7, 3, GL_FLOAT, GL_FALSE, stride,
      reinterpret_cast<const void*>(
          (hasTextureCoordinates ? 8 : 6) * sizeof(float)));
  glBindVertexArray(0);
}

void configureSurfaceVertexArray(unsigned int vertexArray,
                                 unsigned int vertexBuffer,
                                 const std::vector<float>& vertices) {
  configureLitVertexArray(vertexArray, vertexBuffer, vertices, true);
}

}  // namespace

ArenaRenderer::ArenaRenderer(const world::ArenaWorld& world,
                             const std::filesystem::path& playerTexturePath,
                             const std::filesystem::path& zombieTexturePath,
                             const std::filesystem::path& sandTexturePath)
    : playerTexture_(playerTexturePath, Texture::Magnification::Nearest,
                     Texture::Minification::Nearest),
      sandTexture_(sandTexturePath, Texture::Magnification::Linear,
                   Texture::Minification::Linear, Texture::Wrapping::Repeat,
                   Texture::ColorSpace::Srgb),
      sandNormal_(core::assetPath("textures/sand_normal.png"), Texture::Magnification::Linear,
                  Texture::Minification::Linear, Texture::Wrapping::Repeat),
      masonry_(core::assetPath("textures/masonry.png"), Texture::Magnification::Linear,
               Texture::Minification::Linear, Texture::Wrapping::Repeat),
      masonryNormal_(core::assetPath("textures/masonry_normal.png"), Texture::Magnification::Linear,
                     Texture::Minification::Linear, Texture::Wrapping::Repeat) {
  if (std::filesystem::exists(zombieTexturePath)) {
    zombieTexture_.emplace(zombieTexturePath, Texture::Magnification::Nearest,
                           Texture::Minification::Nearest);
  }
  glGenVertexArrays(1, &worldVertexArray_);
  glGenBuffers(1, &worldVertexBuffer_);
  glGenVertexArrays(1, &sandVertexArray_);
  glGenBuffers(1, &sandVertexBuffer_);
  glGenVertexArrays(1, &waterVertexArray_);
  glGenBuffers(1, &waterVertexBuffer_);
  glGenVertexArrays(1, &playerVertexArray_);
  glGenBuffers(1, &playerVertexBuffer_);
  glGenVertexArrays(1, &zombieVertexArray_);
  glGenBuffers(1, &zombieVertexBuffer_);
  glGenBuffers(1, &zombieInstanceBuffer_);

  glGenVertexArrays(1, &blastVertexArray_);
  glGenBuffers(1, &blastVertexBuffer_);
  std::vector<float> ring;
  for (int i = 0; i < 64; ++i) {
    const float angle = 6.2831853F * i / 64;
    appendVertex(ring, {std::cos(angle), 0, std::sin(angle)}, glm::vec3(1));
  }
  configureVertexArray(blastVertexArray_, blastVertexBuffer_, ring);
  rebuildWorldMesh(world);

  std::vector<float> playerVertices;
  playerVertices.reserve(kPlayerPartCount * 36 * 11);
  for (std::size_t part = 0; part < kPlayerSkinLayouts.size(); ++part) {
    playerPartRanges_[part].firstVertex =
        static_cast<int>(playerVertices.size() / 11);
    appendTexturedBox(playerVertices, kPlayerSkinLayouts[part],
                      playerTexture_.width(), playerTexture_.height());
    playerPartRanges_[part].vertexCount = 36;
  }
  configureLitVertexArray(playerVertexArray_, playerVertexBuffer_,
                          playerVertices, true);

  std::vector<float> zombieVertices;
  zombieVertices.reserve(kZombiePartCount * 36 * 11);
  for (std::size_t part = 0; part < kZombieSkinLayouts.size(); ++part) {
    zombiePartRanges_[part].firstVertex =
        static_cast<int>(zombieVertices.size() / 11);
    appendTexturedBox(zombieVertices, kZombieSkinLayouts[part], 64, 64);
    zombiePartRanges_[part].vertexCount = 36;
  }
  configureLitVertexArray(zombieVertexArray_, zombieVertexBuffer_,
                          zombieVertices, true);
  glBindVertexArray(zombieVertexArray_);
  glBindBuffer(GL_ARRAY_BUFFER, zombieInstanceBuffer_);
  for (int column = 0; column < 4; ++column) {
    const GLuint location = static_cast<GLuint>(3 + column);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(
        location, 4, GL_FLOAT, GL_FALSE,
        static_cast<GLsizei>(sizeof(glm::mat4)),
        reinterpret_cast<const void*>(
            static_cast<std::uintptr_t>(column) * sizeof(glm::vec4)));
    glVertexAttribDivisor(location, 1);
  }
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

ArenaRenderer::~ArenaRenderer() {
  glDeleteBuffers(1, &blastVertexBuffer_);
  glDeleteVertexArrays(1, &blastVertexArray_);
  if (playerVertexBuffer_ != 0) {
    glDeleteBuffers(1, &playerVertexBuffer_);
  }
  if (playerVertexArray_ != 0) {
    glDeleteVertexArrays(1, &playerVertexArray_);
  }
  if (zombieVertexBuffer_ != 0) {
    glDeleteBuffers(1, &zombieVertexBuffer_);
  }
  if (zombieInstanceBuffer_ != 0) {
    glDeleteBuffers(1, &zombieInstanceBuffer_);
  }
  if (zombieVertexArray_ != 0) {
    glDeleteVertexArrays(1, &zombieVertexArray_);
  }
  if (worldVertexBuffer_ != 0) {
    glDeleteBuffers(1, &worldVertexBuffer_);
  }
  if (worldVertexArray_ != 0) {
    glDeleteVertexArrays(1, &worldVertexArray_);
  }
  if (sandVertexBuffer_ != 0) {
    glDeleteBuffers(1, &sandVertexBuffer_);
  }
  if (sandVertexArray_ != 0) {
    glDeleteVertexArrays(1, &sandVertexArray_);
  }
  if (waterVertexBuffer_ != 0) {
    glDeleteBuffers(1, &waterVertexBuffer_);
  }
  if (waterVertexArray_ != 0) {
    glDeleteVertexArrays(1, &waterVertexArray_);
  }
}

void ArenaRenderer::rebuildWorldMesh(const world::ArenaWorld& arenaWorld) {
  std::vector<float> solidVertices;
  std::vector<float> sandVertices;
  std::vector<float> waterVertices;
  solidVertices.reserve(32 * 32 * 6 * 6 * 11);
  sandVertices.reserve(32 * 32 * 6 * 11);
  waterVertices.reserve(32 * 32 * 6 * 11);

  for (int y = 0; y < world::ArenaWorld::kHeight; ++y) {
    for (int z = 0; z < world::ArenaWorld::kDepth; ++z) {
      for (int x = 0; x < world::ArenaWorld::kWidth; ++x) {
        const world::BlockType type = arenaWorld.blockAt(x, y, z);
        if (type == world::BlockType::Air) continue;
        appendBlock(solidVertices, arenaWorld, x, y, z, type);
        if (type != world::BlockType::Floor || y != 0 ||
            arenaWorld.blockAt(x, 1, z) != world::BlockType::Air)
          continue;
        if (arenaWorld.isWaterSurface(x, z))
          appendTerrainQuad(waterVertices, x, z,
                            {0.0027F, 0.0509F, 0.2957F},
                            kSandTextureTileWorldSize);
        else
          appendTerrainQuad(sandVertices, x, z, glm::vec3{1.0F},
                            kSandTextureTileWorldSize);
      }
    }
  }

  configureSurfaceVertexArray(worldVertexArray_, worldVertexBuffer_, solidVertices);
  configureSurfaceVertexArray(sandVertexArray_, sandVertexBuffer_,
                              sandVertices);
  configureSurfaceVertexArray(waterVertexArray_, waterVertexBuffer_,
                              waterVertices);
  const int solidVertexCount = static_cast<int>(solidVertices.size() / 11);
  sandVertexCount_ = static_cast<int>(sandVertices.size() / 11);
  waterVertexCount_ = static_cast<int>(waterVertices.size() / 11);
  worldVertexCount_ = solidVertexCount + sandVertexCount_ + waterVertexCount_;
  worldDrawCallCount_ = (solidVertexCount > 0 ? 1 : 0) +
                        (sandVertexCount_ > 0 ? 1 : 0) +
                        (waterVertexCount_ > 0 ? 1 : 0);
}

void ArenaRenderer::drawWorld(const Shader& shader,
                              const glm::mat4& viewProjection,
                              const glm::vec3& eyePosition, world::ArenaLevel level,
                              const glm::vec3& playerPosition, bool normalMapping) const {
  shader.use();
  shader.setMat4("uMvp", viewProjection);
  shader.setMat4("uModel", glm::mat4{1.0F});
  shader.setVec3("uTint", glm::vec3{1.0F});
  shader.setInt("uInstanced", 0);
  shader.setVec3("uEyePosition", eyePosition);
  shader.setVec3("uLightDirection", {-0.35F, 1.0F, 0.25F});
  const bool night = level == world::ArenaLevel::Night;
  shader.setInt("uPointLight", night);
  shader.setVec3("uLightPosition", playerPosition + glm::vec3(0.0F, 2.8F, 0.0F));
  shader.setVec3("uLightColor", night ? glm::vec3(2.15F, 1.65F, 1.12F)
                                     : glm::vec3(1.0F, 0.94F, 0.82F));
  shader.setVec3("uAmbientColor", night ? glm::vec3(0.045F, 0.065F, 0.12F)
                                       : glm::vec3(0.25F, 0.30F, 0.38F));
  shader.setInt("uNormalMapping", normalMapping);
  shader.setInt("uNormalMap", 1);
  shader.setInt("uTexture", 0);
  shader.setInt("uMaterialType", 4);  // Masonry
  shader.setInt("uUseTexture", 1);
  masonryNormal_.bind(1);
  masonry_.bind(0);

  // All terrain triangles use outward, counter-clockwise winding.
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  const int solidVertexCount =
      worldVertexCount_ - sandVertexCount_ - waterVertexCount_;
  if (solidVertexCount > 0) {
    glBindVertexArray(worldVertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, solidVertexCount);
  }

  if (sandVertexCount_ > 0) {
    shader.setInt("uMaterialType", 1);
    shader.setInt("uUseTexture", 1);
    shader.setInt("uTexture", 0);
    shader.setFloat("uMetallic", 0.0F);
    shader.setFloat("uRoughness", 0.88F);
    sandNormal_.bind(1);
    sandTexture_.bind();
    glBindVertexArray(sandVertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, sandVertexCount_);
  }

  if (waterVertexCount_ > 0) {
    shader.setInt("uMaterialType", 2);
    shader.setInt("uUseTexture", 0);
    shader.setFloat("uMetallic", 0.0F);
    shader.setFloat("uRoughness", 0.20F);
    shader.setFloat("uWaterIor", 1.33F);
    shader.setVec3("uWaterReflectionColor", night ? glm::vec3(0.09F, 0.14F, 0.24F)
                                                 : glm::vec3(0.55F, 0.78F, 0.96F));
    glBindVertexArray(waterVertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, waterVertexCount_);
  }
  glDisable(GL_CULL_FACE);
  glBindVertexArray(0);
  shader.setInt("uMaterialType", 0);
  shader.setInt("uUseTexture", 0);
}

void ArenaRenderer::drawPlayer(const Shader& shader,
                               const glm::mat4& viewProjection,
                               const glm::vec3& position, float yawRadians,
                               float walkCyclePhase, bool moving) const {
  glm::mat4 actorRoot{1.0F};
  actorRoot = glm::translate(actorRoot, position);
  actorRoot = glm::rotate(actorRoot, yawRadians, glm::vec3{0.0F, 1.0F, 0.0F});
  const glm::mat4 feet = glm::scale(
      glm::translate(actorRoot, glm::vec3{0.0F, -kPlayerFeetOffset, 0.0F}),
      glm::vec3(gameplay::kActorScale));

  const glm::vec3 headSize{8.0F * kSkinPixelSize};
  const glm::vec3 torsoSize{8.0F * kSkinPixelSize, 12.0F * kSkinPixelSize,
                            4.0F * kSkinPixelSize};
  const glm::vec3 limbSize{4.0F * kSkinPixelSize, 12.0F * kSkinPixelSize,
                           4.0F * kSkinPixelSize};
  const float swing =
      moving ? std::sin(walkCyclePhase) * kWalkSwingAmplitude : 0.0F;

  const auto makeBoxModel = [&feet](const glm::vec3& center,
                                    const glm::vec3& size) {
    return glm::scale(glm::translate(feet, center), size);
  };
  const auto makeLimbModel = [&feet](const glm::vec3& joint,
                                     const glm::vec3& size, float angle) {
    glm::mat4 model = glm::translate(feet, joint);
    model = glm::rotate(model, angle, glm::vec3{1.0F, 0.0F, 0.0F});
    model = glm::translate(model, glm::vec3{0.0F, -size.y * 0.5F, 0.0F});
    return glm::scale(model, size);
  };

  const auto drawPart = [&](std::size_t part, const glm::mat4& model) {
    shader.setMat4("uMvp", viewProjection * model);
    shader.setMat4("uModel", model);
    const PlayerPartRange& range = playerPartRanges_[part];
    glDrawArrays(GL_TRIANGLES, range.firstVertex, range.vertexCount);
  };

  shader.use();
  shader.setVec3("uTint", glm::vec3{1.0F});
  shader.setInt("uTexture", 0);
  shader.setInt("uUseTexture", 1);
  shader.setInt("uInstanced", 0);
  shader.setInt("uMaterialType", 0);
  playerTexture_.bind();
  glBindVertexArray(playerVertexArray_);

  drawPart(0, makeBoxModel({0.0F, 28.0F * kSkinPixelSize, 0.0F}, headSize));
  drawPart(1, makeBoxModel({0.0F, 18.0F * kSkinPixelSize, 0.0F}, torsoSize));

  const float shoulderY = 24.0F * kSkinPixelSize;
  const float shoulderX = 6.0F * kSkinPixelSize;
  const float hipY = 12.0F * kSkinPixelSize;
  const float hipX = 2.0F * kSkinPixelSize;
  drawPart(2, makeLimbModel({-shoulderX, shoulderY, 0.0F}, limbSize, -swing));
  drawPart(3, makeLimbModel({shoulderX, shoulderY, 0.0F}, limbSize, swing));
  drawPart(4, makeLimbModel({-hipX, hipY, 0.0F}, limbSize, swing));
  drawPart(5, makeLimbModel({hipX, hipY, 0.0F}, limbSize, -swing));

  glBindVertexArray(0);
}

void ArenaRenderer::drawZombies(
    const Shader& shader, const glm::mat4& viewProjection,
    const std::vector<ZombieInstance>& zombies) const {
  if (zombies.empty()) return;

  const glm::vec3 headSize{8.0F * kSkinPixelSize};
  const glm::vec3 torsoSize{8.0F * kSkinPixelSize, 12.0F * kSkinPixelSize,
                            4.0F * kSkinPixelSize};
  const glm::vec3 limbSize{4.0F * kSkinPixelSize, 12.0F * kSkinPixelSize,
                           4.0F * kSkinPixelSize};
  const float shoulderY = 24.0F * kSkinPixelSize;
  const float shoulderX = 6.0F * kSkinPixelSize;
  const float hipY = 12.0F * kSkinPixelSize;
  const float hipX = 2.0F * kSkinPixelSize;
  std::array<std::vector<glm::mat4>, kZombiePartCount> partModels;
  for (auto& models : partModels) models.reserve(zombies.size());

  for (const ZombieInstance& zombie : zombies) {
    glm::mat4 actorRoot{1.0F};
    actorRoot = glm::translate(actorRoot, zombie.position);
    actorRoot = glm::rotate(actorRoot, zombie.yawRadians,
                            glm::vec3{0.0F, 1.0F, 0.0F});
    const glm::mat4 feet = glm::scale(
        glm::translate(actorRoot, glm::vec3{0.0F, -kPlayerFeetOffset, 0.0F}),
        glm::vec3(gameplay::kActorScale));
    const auto makeBoxModel = [&feet](const glm::vec3& center,
                                      const glm::vec3& size) {
      return glm::scale(glm::translate(feet, center), size);
    };
    const auto makeLimbModel = [&feet](const glm::vec3& joint,
                                       const glm::vec3& size, float angle) {
      glm::mat4 model = glm::translate(feet, joint);
      model = glm::rotate(model, angle, glm::vec3{1.0F, 0.0F, 0.0F});
      model = glm::translate(model, glm::vec3{0.0F, -size.y * 0.5F, 0.0F});
      return glm::scale(model, size);
    };

    const float swing = zombie.moving
                            ? std::sin(zombie.walkCyclePhase) *
                                  kWalkSwingAmplitude
                            : 0.0F;
    float armRaise = 0.0F;
    if (zombie.attackAnimationTime >= 0.0F) {
      const float progress = std::clamp(
          zombie.attackAnimationTime / kZombieAttackAnimationSeconds, 0.0F,
          1.0F);
      const auto smoothStep = [](float value) {
        value = std::clamp(value, 0.0F, 1.0F);
        return value * value * (3.0F - 2.0F * value);
      };
      if (progress < 0.25F)
        armRaise = smoothStep(progress / 0.25F);
      else if (progress < 0.52F)
        armRaise = 1.0F;
      else
        armRaise = 1.0F - smoothStep((progress - 0.52F) / 0.48F);
    }
    const float attackArmAngle = kZombieArmForwardAngle - 0.90F * armRaise;

    partModels[0].push_back(
        makeBoxModel({0.0F, 28.0F * kSkinPixelSize, 0.0F}, headSize));
    partModels[1].push_back(
        makeBoxModel({0.0F, 18.0F * kSkinPixelSize, 0.0F}, torsoSize));
    // Both arms stay extended in front in the classic zombie pose.
    partModels[2].push_back(makeLimbModel(
        {-shoulderX, shoulderY, 0.0F}, limbSize, attackArmAngle));
    partModels[3].push_back(makeLimbModel(
        {shoulderX, shoulderY, 0.0F}, limbSize, attackArmAngle));
    partModels[4].push_back(
        makeLimbModel({-hipX, hipY, 0.0F}, limbSize, swing));
    partModels[5].push_back(
        makeLimbModel({hipX, hipY, 0.0F}, limbSize, -swing));
  }

  std::vector<glm::mat4> instanceModels;
  instanceModels.reserve(zombies.size() * kZombiePartCount);
  for (const auto& models : partModels)
    instanceModels.insert(instanceModels.end(), models.begin(), models.end());

  shader.use();
  shader.setMat4("uViewProjection", viewProjection);
  shader.setInt("uInstanced", 1);
  shader.setVec3("uTint", zombieTexture_ ? glm::vec3{1.0F}
                                          : glm::vec3{0.24F, 0.78F, 0.32F});
  shader.setInt("uTexture", 0);
  shader.setInt("uUseTexture", zombieTexture_ ? 1 : 0);
  shader.setInt("uMaterialType", 0);
  shader.setMat4("uModel", glm::mat4{1.0F});
  if (zombieTexture_) zombieTexture_->bind();

  glBindVertexArray(zombieVertexArray_);
  glBindBuffer(GL_ARRAY_BUFFER, zombieInstanceBuffer_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(instanceModels.size() *
                                       sizeof(glm::mat4)),
               instanceModels.data(), GL_STREAM_DRAW);
  for (std::size_t part = 0; part < partModels.size(); ++part) {
    const std::size_t partOffset =
        part * zombies.size() * sizeof(glm::mat4);
    for (int column = 0; column < 4; ++column) {
      const GLuint location = static_cast<GLuint>(3 + column);
      const std::size_t offset =
          partOffset + static_cast<std::size_t>(column) * sizeof(glm::vec4);
      glVertexAttribPointer(
          location, 4, GL_FLOAT, GL_FALSE,
          static_cast<GLsizei>(sizeof(glm::mat4)),
          reinterpret_cast<const void*>(static_cast<std::uintptr_t>(offset)));
    }
    const PlayerPartRange& range = zombiePartRanges_[part];
    glDrawArraysInstanced(GL_TRIANGLES, range.firstVertex, range.vertexCount,
                          static_cast<GLsizei>(zombies.size()));
  }
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ArenaRenderer::drawProxy(const Shader& shader, const glm::mat4& vp,
                              const glm::vec3& position,
                              const glm::vec3& halfExtents,
                              const glm::vec3& color, float yawRadians, bool metallic) const {
  shader.use();
  shader.setInt("uUseTexture", 0);
  shader.setInt("uInstanced", 0);
  shader.setInt("uMaterialType", metallic ? 5 : 0);
  shader.setVec3("uTint", color);
  const glm::mat4 model =
      glm::scale(glm::rotate(glm::translate(glm::mat4(1), position),
                             yawRadians, glm::vec3(0, 1, 0)),
                 halfExtents * 2.0F);
  shader.setMat4("uModel", model);
  shader.setMat4("uMvp", vp * model);
  glBindVertexArray(playerVertexArray_);
  glDrawArrays(GL_TRIANGLES, playerPartRanges_[0].firstVertex, 36);
  glBindVertexArray(0);
}

void ArenaRenderer::drawBlast(const Shader& shader, const glm::mat4& vp,
                              const glm::vec3& position, float radius,
                              float age) const {
  shader.use();
  shader.setInt("uUseTexture", 0);
  shader.setInt("uInstanced", 0);
  shader.setInt("uMaterialType", 3);
  shader.setVec3("uTint", glm::vec3(1.F, 0.55F, 0.12F) * (1 - age / 0.4F));
  // Full damage radius is visible immediately; this is a debug ring, not M3
  // particles.
  const glm::mat4 model = glm::scale(
      glm::translate(glm::mat4(1), glm::vec3(position.x, 1.06F, position.z)),
      glm::vec3(radius, 1, radius));
  shader.setMat4("uModel", model);
  shader.setMat4("uMvp", vp * model);
  glBindVertexArray(blastVertexArray_);
  glDrawArrays(GL_LINE_LOOP, 0, 64);
  glBindVertexArray(0);
}

int ArenaRenderer::worldVertexCount() const noexcept {
  return worldVertexCount_;
}

int ArenaRenderer::worldDrawCallCount() const noexcept {
  return worldDrawCallCount_;
}

}  // namespace craft::graphics

#include "graphics/ArenaRenderer.h"

#include <array>
#include <cmath>
#include <vector>

#include <glm/ext/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "graphics/OpenGL.h"
#include "graphics/Shader.h"
#include "world/ArenaWorld.h"

namespace craft::graphics {

namespace {

struct FaceDefinition {
  int neighborX;
  int neighborY;
  int neighborZ;
  float shade;
  std::array<glm::vec3, 4> corners;
};

const std::array<FaceDefinition, 6> kCubeFaces{{
    // +X
    {1, 0, 0, 0.78F,
     {{{1.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 0.0F},
       {1.0F, 1.0F, 1.0F}, {1.0F, 0.0F, 1.0F}}}},
    // -X
    {-1, 0, 0, 0.68F,
     {{{0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 1.0F},
       {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 0.0F}}}},
    // +Y
    {0, 1, 0, 1.12F,
     {{{0.0F, 1.0F, 0.0F}, {0.0F, 1.0F, 1.0F},
       {1.0F, 1.0F, 1.0F}, {1.0F, 1.0F, 0.0F}}}},
    // -Y
    {0, -1, 0, 0.52F,
     {{{0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 0.0F},
       {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 1.0F}}}},
    // +Z
    {0, 0, 1, 0.90F,
     {{{0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 1.0F},
       {1.0F, 1.0F, 1.0F}, {0.0F, 1.0F, 1.0F}}}},
    // -Z
    {0, 0, -1, 0.74F,
     {{{1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F},
       {0.0F, 1.0F, 0.0F}, {1.0F, 1.0F, 0.0F}}}},
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
    {16.0F, 8.0F, 8.0F, 8.0F},   // left
    {0.0F, 8.0F, 8.0F, 8.0F},    // right
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
    {4.0F, 16.0F, 4.0F, 4.0F},   // top
    {8.0F, 16.0F, 4.0F, 4.0F},   // bottom
    {4.0F, 20.0F, 4.0F, 12.0F},  // front
    {12.0F, 20.0F, 4.0F, 12.0F}, // back
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
constexpr float kSkinPixelSize = 0.05F;
constexpr float kPlayerFeetOffset = 0.45F;
constexpr float kWalkSwingAmplitude = 0.55F;

void appendVertex(std::vector<float>& vertices, const glm::vec3& position,
                  const glm::vec3& color) {
  vertices.insert(vertices.end(),
                  {position.x, position.y, position.z, color.r, color.g,
                   color.b});
}

void appendTexturedVertex(std::vector<float>& vertices,
                          const glm::vec3& position, const glm::vec3& color,
                          const glm::vec2& uv) {
  vertices.insert(vertices.end(), {position.x, position.y, position.z, color.r,
                                   color.g, color.b, uv.x, uv.y});
}

void appendQuad(std::vector<float>& vertices,
                const std::array<glm::vec3, 4>& corners,
                const glm::vec3& color) {
  constexpr int kTriangleCorners[] = {0, 1, 2, 0, 2, 3};
  for (const int cornerIndex : kTriangleCorners) {
    appendVertex(vertices, corners[cornerIndex], color);
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
    for (const int cornerIndex : kTriangleCorners) {
      const glm::vec3 position = origin + face.corners[cornerIndex];
      const glm::vec3 faceShade{face.shade};
      appendTexturedVertex(vertices, position, faceShade, uvs[cornerIndex]);
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
    if (arenaWorld.blockAt(x + face.neighborX, y + face.neighborY,
                           z + face.neighborZ) != world::BlockType::Air) {
      continue;
    }

    std::array<glm::vec3, 4> worldCorners;
    for (std::size_t i = 0; i < face.corners.size(); ++i) {
      worldCorners[i] =
          origin + face.corners[i] * world::ArenaWorld::kBlockSize;
    }
    appendQuad(vertices, worldCorners, baseColor * face.shade);
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

}  // namespace

ArenaRenderer::ArenaRenderer(
    const world::ArenaWorld& world,
    const std::filesystem::path& playerTexturePath)
    : playerTexture_(playerTexturePath, Texture::Magnification::Nearest,
                     Texture::Minification::Nearest) {
  glGenVertexArrays(1, &worldVertexArray_);
  glGenBuffers(1, &worldVertexBuffer_);
  glGenVertexArrays(1, &playerVertexArray_);
  glGenBuffers(1, &playerVertexBuffer_);

  rebuildWorldMesh(world);

  std::vector<float> playerVertices;
  playerVertices.reserve(kPlayerPartCount * 36 * 8);
  for (std::size_t part = 0; part < kPlayerSkinLayouts.size(); ++part) {
    playerPartRanges_[part].firstVertex =
        static_cast<int>(playerVertices.size() / 8);
    appendTexturedBox(playerVertices, kPlayerSkinLayouts[part],
                      playerTexture_.width(), playerTexture_.height());
    playerPartRanges_[part].vertexCount = 36;
  }
  configureVertexArray(playerVertexArray_, playerVertexBuffer_, playerVertices,
                       true);
}

ArenaRenderer::~ArenaRenderer() {
  if (playerVertexBuffer_ != 0) {
    glDeleteBuffers(1, &playerVertexBuffer_);
  }
  if (playerVertexArray_ != 0) {
    glDeleteVertexArrays(1, &playerVertexArray_);
  }
  if (worldVertexBuffer_ != 0) {
    glDeleteBuffers(1, &worldVertexBuffer_);
  }
  if (worldVertexArray_ != 0) {
    glDeleteVertexArrays(1, &worldVertexArray_);
  }
}

void ArenaRenderer::rebuildWorldMesh(const world::ArenaWorld& arenaWorld) {
  std::vector<float> vertices;
  vertices.reserve(32 * 32 * 6 * 6 * 6);

  for (int y = 0; y < world::ArenaWorld::kHeight; ++y) {
    for (int z = 0; z < world::ArenaWorld::kDepth; ++z) {
      for (int x = 0; x < world::ArenaWorld::kWidth; ++x) {
        const world::BlockType type = arenaWorld.blockAt(x, y, z);
        if (type != world::BlockType::Air) {
          appendBlock(vertices, arenaWorld, x, y, z, type);
        }
      }
    }
  }

  worldVertexCount_ = static_cast<int>(vertices.size() / 6);
  configureVertexArray(worldVertexArray_, worldVertexBuffer_, vertices);
}

void ArenaRenderer::drawWorld(const Shader& shader,
                              const glm::mat4& viewProjection) const {
  shader.use();
  shader.setMat4("uMvp", viewProjection);
  shader.setVec3("uTint", glm::vec3{1.0F});
  shader.setInt("uUseTexture", 0);
  glBindVertexArray(worldVertexArray_);
  glDrawArrays(GL_TRIANGLES, 0, worldVertexCount_);
}

void ArenaRenderer::drawPlayer(const Shader& shader,
                               const glm::mat4& viewProjection,
                               const glm::vec3& position,
                               float yawRadians, float walkCyclePhase,
                               bool moving) const {
  glm::mat4 actorRoot{1.0F};
  actorRoot = glm::translate(actorRoot, position);
  actorRoot =
      glm::rotate(actorRoot, yawRadians, glm::vec3{0.0F, 1.0F, 0.0F});
  const glm::mat4 feet = glm::translate(
      actorRoot, glm::vec3{0.0F, -kPlayerFeetOffset, 0.0F});

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
    const PlayerPartRange& range = playerPartRanges_[part];
    glDrawArrays(GL_TRIANGLES, range.firstVertex, range.vertexCount);
  };

  shader.use();
  shader.setVec3("uTint", glm::vec3{1.0F});
  shader.setInt("uTexture", 0);
  shader.setInt("uUseTexture", 1);
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

int ArenaRenderer::worldVertexCount() const noexcept {
  return worldVertexCount_;
}

}  // namespace craft::graphics

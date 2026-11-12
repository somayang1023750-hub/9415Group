#include "graphics/Texture.h"

#include <stdexcept>

#include <stb_image.h>

#include "graphics/OpenGL.h"

namespace craft::graphics {

Texture::Texture(const std::filesystem::path& path,
                 Magnification magnification,
                 Minification minification, Wrapping wrapping,
                 ColorSpace colorSpace) {
  stbi_set_flip_vertically_on_load(0);
  int channels = 0;
  unsigned char* pixels =
      stbi_load(path.string().c_str(), &width_, &height_, &channels, 4);
  if (pixels == nullptr) {
    throw std::runtime_error("Failed to load texture: " + path.string());
  }

  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  const GLint internalFormat = colorSpace == ColorSpace::Srgb
                                   ? GL_SRGB8_ALPHA8
                                   : GL_RGBA8;
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width_, height_, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels);
  stbi_image_free(pixels);

  // Mipmaps reduce minification aliasing; the sampling mode controls blending.
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  minification == Minification::Nearest
                      ? GL_NEAREST_MIPMAP_NEAREST
                      : GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  magnification == Magnification::Nearest ? GL_NEAREST
                                                          : GL_LINEAR);
  const GLint wrapMode =
      wrapping == Wrapping::Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
  glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() {
  if (texture_ != 0) {
    glDeleteTextures(1, &texture_);
  }
}

void Texture::bind(unsigned int unit) const {
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, texture_);
}

}  // namespace craft::graphics

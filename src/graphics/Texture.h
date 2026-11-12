#pragma once

#include <filesystem>

namespace craft::graphics {

// Owns one 2D OpenGL texture decoded as linear RGBA8 or sRGB RGBA8.
// A valid OpenGL context must exist for the lifetime of each Texture.
class Texture {
 public:
  // How the texture is sampled when drawn larger than its pixel size.
  enum class Magnification {
    Linear,   // Smooth, suits fonts and photos.
    Nearest,  // Keeps hard pixel edges, suits pixel art.
  };

  // How texture samples are combined when a surface is drawn smaller.
  enum class Minification {
    Linear,   // Smooth mipmap filtering.
    Nearest,  // Preserve pixel-art texels across mip levels.
  };

  enum class Wrapping {
    ClampToEdge,
    Repeat,
  };

  // Base-colour images use sRGB decoding; data maps stay linear.
  enum class ColorSpace {
    Linear,
    Srgb,
  };

  // Loads the image with its first row at v = 0, so UVs follow image pixels.
  explicit Texture(const std::filesystem::path& path,
                   Magnification magnification = Magnification::Linear,
                   Minification minification = Minification::Linear,
                   Wrapping wrapping = Wrapping::ClampToEdge,
                   ColorSpace colorSpace = ColorSpace::Linear);
  ~Texture();

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  // Binds this texture to the requested texture unit.
  void bind(unsigned int unit = 0) const;

  [[nodiscard]] int width() const noexcept { return width_; }
  [[nodiscard]] int height() const noexcept { return height_; }

 private:
  unsigned int texture_ = 0;
  int width_ = 0;
  int height_ = 0;
};

}  // namespace craft::graphics

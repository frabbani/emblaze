#pragma once

#include <vector>
#include <string_view>
#include <string>
#include <optional>
#include <algorithm>
#include <memory>

#include "../color.h"

namespace mbz {
namespace utils {
namespace img {

struct Image {
  std::optional<std::string> source = std::nullopt;
  int w = 0, h = 0;
  std::vector<Color> pixels;
  std::unique_ptr<Image> mip = nullptr;
  Image() = default;
  ~Image() = default;
  bool isValid() const {
    return w > 0 && h > 0 && pixels.size() == size_t(w * h);
  }

  Image(const Image& from);

  Color get(int x, int y, bool clamp = true) const {
    if (!clamp) {
      x %= w;
      y %= h;
      x += x < 0 ? w : 0;
      y += y < 0 ? h : 0;
    } else {
      x = std::clamp(x, 0, w - 1);
      y = std::clamp(y, 0, h - 1);
    }
    return pixels.size() ? pixels[y * w + x] : Color();
  }


  void put(int x, int y, Color color, bool clamp = true) {
    if (!clamp) {
      x %= w;
      y %= h;
      x += x < 0 ? w : 0;
      y += y < 0 ? h : 0;
    } else {
      x = std::clamp(x, 0, w - 1);
      y = std::clamp(y, 0, h - 1);
    }
    if (pixels.size())
      pixels[y * w + x] = color;

  }
  void createMips();

  Color sample(float x, float y, bool clamp = false) const;
  Color sampleBox(int x, int y, int w, int h, bool clamp = false) const;
  Color sampleMipmap(float x, float y, int level, bool clamp = false) const;

  void fromBMP(std::vector<uint8_t> rawData, std::string_view source);
};

int computeMipmapLevel(float sourceArea, float targetArea);

bool writeImageToBMPFile(const Image &image, std::string_view name);

}
}
}

#pragma once

#include "../math/vector.h"
#include <string_view>
#include "../utils/image.h"


namespace mbz{
namespace rasterizer{

bool loadTexture(const mbz::utils::img::Image &image, std::string_view tag, int &handle);

int getTextureHandle(std::string_view tag);

struct Texel : public Color {
  int textureHandle = -1;
  int mipLevel = 0;
  mbz::math::Vector2 uv;
  Color sample() const;
};

}
}

#pragma once

#include <text.h>
#include <string_view>

struct AsciiCharSet : public MyGL_AsciiCharSet {
  AsciiCharSet(std::string_view name);
  ~AsciiCharSet();
};

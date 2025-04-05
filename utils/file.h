#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mbz {
namespace utils {

struct FileData {
  std::string name;
  std::vector<uint8_t> data;
  FileData() = default;
  FileData(std::string_view fileName);
  void save(std::string_view fileName);
};

}
}

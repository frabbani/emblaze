#include <cstdio>

#include "file.h"

namespace mbz {
namespace utils {

FileData::FileData(std::string_view fileName) {
  FILE *fp = fopen(fileName.data(), "rb");
  if (!fp) {
    return;
  }
  fseek(fp, 0, SEEK_END);
  auto size = ftell(fp);
  if (!size) {
    fclose(fp);
    return;
  }
  name = fileName;
  data = std::vector<uint8_t>(size);
  fseek(fp, 0, SEEK_SET);
  fread( data.data(), size, 1, fp );
  fclose(fp);
}

void FileData::save(std::string_view fileName) {
  FILE *fp = fopen(fileName.data(), "rb");
  fwrite(data.data(), data.size(), 1, fp);
  fclose(fp);
}

}
}

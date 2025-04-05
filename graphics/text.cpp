#include "text.h"

#include <cstdio>
#include <cstring>
#include <vector>

AsciiCharSet::AsciiCharSet(std::string_view name) {

  auto load_file = [](std::vector<uint8_t>& data, const char *fn){
    data.clear();
    FILE *fp = fopen(fn, "rb");
    if(fp){
      fseek(fp, 0, SEEK_END);
      long fileSize = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      if (fileSize > 0) {
        data.resize(fileSize);
        fread(data.data(), 1, fileSize, fp);
      }
      fclose(fp);
    }
  };

  std::vector<uint8_t> data;
  load_file(data, MyGL_str64Fmt("assets/fonts/%s/glyphs.bmp", name.data()).chars);
  imageAtlas = MyGL_imageFromBMPData(&data[0], data.size(), name.data());
  if (!imageAtlas.pixels) {
    printf("failed to load font '%s' bitmap\n", name.data());
    return;
  }
  this->name = MyGL_str64(name.data());

  numChars = 0;
  for (int i = 0; i < MYGL_NUM_ASCII_PRINTABLE_CHARS; i++) {
    chars[i].c = 0;
    chars[i].x = chars[i].y = chars[i].w = chars[i].h = 0.0f;
  }

  FILE *fp = fopen(MyGL_str64Fmt("assets/fonts/%s/glyphs.txt", name).chars, "r");
  if (!fp)
    return;
  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    if (line[0] >= ' ' && line[0] <= '~') {
      int i = numChars;
      chars[i].c = line[0];
      sscanf(&line[2], "%u %u %u %u", &chars[i].w, &chars[i].h, &chars[i].x, &chars[i].y);
//        printf("'%c' %u x %u < %u , %u >\n", chars[i].c, chars[i].w, chars[i].h,
//               chars[i].x, chars[i].y);
      numChars++;
    }
  }
  fclose(fp);
}

AsciiCharSet::~AsciiCharSet() {
  MyGL_imageFree(&imageAtlas);
  memset(this, 0, sizeof(MyGL_AsciiCharSet));
}

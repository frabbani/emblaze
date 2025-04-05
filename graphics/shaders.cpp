#include <mygl.h>

#include <sstream>
#include "shaders.h"
#include "../utils/log.h"

struct GetCharParam {
  std::string str;
  size_t pos = 0;
};

char getCharCb(void *param) {
  GetCharParam *p = (GetCharParam*) param;
  if (p->pos >= p->str.length())
    return '\0';
  return p->str.c_str()[p->pos++];
}

void loadShaderLibraries() {
  auto makeCbParam = [=](const char *filename) {
    FILE *fp = fopen(filename, "r");
    std::stringstream ss;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
      ss << line;
    }
    GetCharParam p;
    p.str = ss.str();
    return p;
  };

  LOGINFO("loadShaderLibraries", "---------");
  auto param = makeCbParam("assets/shaders/includes.glsl");
  MyGL_loadShaderLibrary(getCharCb, &param, "includes.glsl");
  LOGINFO("loadShaderLibraries", "---------");
  param = makeCbParam("assets/shaders/colored.shader");
  MyGL_loadShader(getCharCb, &param, "colored.shader");
  LOGINFO("loadShaderLibraries", "---------");
  param = makeCbParam("assets/shaders/textured.shader");
  MyGL_loadShader(getCharCb, &param, "textured.shader");
  LOGINFO("loadShaderLibraries", "---------");
}

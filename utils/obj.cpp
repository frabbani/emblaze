#include <cstring>
#include <string>
#include <set>
#include <cstdarg>
#include <functional>

#include "obj.h"
#include "file.h"
#include "log.h"

namespace mbz {

using namespace mbz::math;

namespace utils {

using namespace logger;

namespace wavefront {

bool OBJ::Vertex::operator <(const Vertex &rhs) const {
  if (p < rhs.p)
    return true;
  if (p > rhs.p)
    return false;

  if (n < rhs.n)
    return true;
  if (n > rhs.n)
    return false;

  if (t < rhs.t)
    return true;
  if (t > rhs.t)
    return false;

  return false;
}

bool OBJ::Vertex::operator >(const Vertex &rhs) const {
  return rhs < *this;
}

bool OBJ::Vertex::operator ==(const Vertex &rhs) const {
  return p == rhs.p && t == rhs.t && t == rhs.t;
}

bool OBJ::load(std::string_view fileName) {
  const char *tag = "OBJ::Load";

  FileData fd(fileName);

  FILE *fp = fopen(fileName.data(), "r");

  if (!fp) {
    LOGERROR(tag, "file '%s' not found", fileName.data());
    return false;
  }
  LOGINFO(tag, "loading OBJ from file '%s'", fileName.data());

  pts.clear();
  uvs.clear();
  mats.clear();
  faces.clear();
  materialFaces.clear();

  char line[256];

  std::string matCurr;
  int matIndex = 0;
  while (fgets(line, sizeof(line), fp)) {
    if ('v' == line[0] && ' ' == line[1]) {
      Vector3 v;
      sscanf(&line[2], "%f %f %f", &v.x, &v.y, &v.z);
      pts.push_back(v);
    }

    if ('v' == line[0] && 'n' == line[1]) {
      Vector3 v;
      sscanf(&line[3], "%f %f %f", &v.x, &v.y, &v.z);
      nos.push_back(v);
    }

    if ('v' == line[0] && 't' == line[1]) {
      Vector2 v;
      sscanf(&line[3], "%f %f", &v.x, &v.y);
      uvs.push_back(v);
    }

    if (0 == memcmp("usemtl", line, 6)) {
      mats.push_back(std::string(strtok(&line[7], " \n")));
      matCurr = mats.back();
      matIndex = (int) mats.size() - 1;
    }
    if ('f' == line[0] && ' ' == line[1]) {
      Face face;
      face.m = matIndex;
      sscanf(strtok(&line[2], " \n"), "%d/%d/%d", &face.v.p, &face.v.uv, &face.v.n);
      sscanf(strtok(nullptr, " \n"), "%d/%d/%d", &face.v2.p, &face.v2.uv, &face.v2.n);
      sscanf(strtok(nullptr, " \n"), "%d/%d/%d", &face.v3.p, &face.v3.uv, &face.v3.n);
      faces.push_back(face);
      materialFaces[matCurr].push_back(faces.size() - 1);
    }
  }
  fclose(fp);

  std::set<Vertex> sortedVerts;
  size_t n = 0;
  for (auto &f : faces) {
    //materialFaces[ mats[f.m] ].push_back(n);
    for (int i = 0; i < 3; i++) {
      f.verts[i].p = pts[f.vs[i].p - 1];
      f.verts[i].n = nos[f.vs[i].n - 1];
      f.verts[i].t = uvs[f.vs[i].uv - 1];
      sortedVerts.insert(f.verts[i]);
    }
    n++;
  }

  // these are now sorted
  for (auto vert : sortedVerts)
    vertices.push_back(vert);

  auto find = [&](const Vertex &v) {
    int l = 0;
    int r = (int) vertices.size() - 1;
    while (l <= r) {
      int m = (l + r) / 2;
      if (v > vertices[m])
        l = m + 1;
      else if (v < vertices[m])
        r = m - 1;
      else
        return m;
    }
    return -1;
  };

  for (auto &face : faces) {
    face.ids[0] = find(face.verts[0]);
    face.ids[1] = find(face.verts[1]);
    face.ids[2] = find(face.verts[2]);
    indices.push_back(face.ids[0]);
    indices.push_back(face.ids[1]);
    indices.push_back(face.ids[2]);
  }

  return true;
}

bool OBJ::loadMaterials(std::string_view fileName) {
  const char *tag = "OBJ::laodMaterials";

  auto from_hex = [](const char *str) {
    Color c;
    c.r = c.g = c.b = 0;
    if (str[0] == '#') {
      uint32_t rgb;
      sscanf(&str[1], "%x", &rgb);
      c.b = rgb & 0xff;
      c.g = (rgb >> 8) & 0xff;
      c.r = (rgb >> 16) & 0xff;
    }
    return c;
  };

  FILE *fp = fopen(fileName.data(), "r");

  if (!fp) {
    LOGERROR(tag, "file '%s' not found", fileName.data());
    return false;
  }
  LOGWARN(tag, "loading OBJ materials from file '%s'", fileName.data());

  char line[256];
  int lineNo = -1;
  while (fgets(line, sizeof(line), fp)) {
    lineNo++;
    if ('#' == line[0])
      continue;
    auto *p = strchr(line, '\n');
    if (p)
      *p = '\0';

    LOGINFO(tag, "%s", line);

    char *toks[3];
    toks[0] = strtok(line, " \n");
    toks[1] = strtok(nullptr, " \n");
    toks[2] = strtok(nullptr, " \n");
    if (toks[0] == nullptr || toks[1] == nullptr || toks[2] == nullptr) {
      LOGWARN(tag, "invalid material at line %d", lineNo);
    }
    auto &material = materials[std::string(toks[0])];
    material.name = std::string(toks[0]);
    material.albedo = from_hex(toks[1]);
    material.emission = from_hex(toks[2]);
    //material.albedo.fromHex(toks[1]);
    //material.emission.fromHex(toks[2]);
  }

  return true;
}

void exportObj(std::string_view fileName, const OBJ &obj) {
  FILE *fp = fopen(fileName.data(), "w");
  if (!fp) {
    LOGERROR(__FUNCTION__, "failed to open file '%s' for writing", fileName.data());
    return;
  }
  fprintf(fp, "# %s - %zu vertices / %zu faces\n", fileName.data(), obj.vertices.size(), obj.faces.size());
  fprintf(fp, "# positions:\n");
  for (auto v : obj.vertices)
    fprintf(fp, "v %f %f %f\n", v.p.x, v.p.y, v.p.z);

  fprintf(fp, "# normals:\n");
  for (auto v : obj.vertices)
    fprintf(fp, "vn %f %f %f\n", v.n.x, v.n.y, v.n.z);

  fprintf(fp, "# uvs:\n");
  for (auto v : obj.vertices)
    fprintf(fp, "vt %f %f\n", v.t.x, v.t.y);

  fprintf(fp, "# faces:\n");
  for (auto f : obj.faces) {
    fprintf(fp, "f");
    for (int i = 0; i < 3; i++) {
      fprintf(fp, " %d/%d/%d", f.ids[i] + 1, f.ids[i] + 1, f.ids[i] + 1);
    }
    fprintf(fp, "\n");
  }

  LOGINFO(__FUNCTION__, "exporting OBJ to file '%s'...", fileName.data());
  LOGINFO(__FUNCTION__, "%s", "done!");
  fclose(fp);
}

void exportObj(std::string_view fileName, OBJ &obj, std::string material) {
  FILE *fp = fopen(fileName.data(), "w");
  if (!fp) {
    LOGERROR(__FUNCTION__, "failed to open file '%s' for writing", fileName.data());
    return;
  }
  fprintf(fp, "# %s - %zu vertices / %zu faces\n", fileName.data(), obj.vertices.size(), obj.faces.size());
  fprintf(fp, "# positions:\n");
  for (auto v : obj.vertices)
    fprintf(fp, "v %f %f %f\n", v.p.x, v.p.y, v.p.z);

  fprintf(fp, "# normals:\n");
  for (auto v : obj.vertices)
    fprintf(fp, "vn %f %f %f\n", v.n.x, v.n.y, v.n.z);

  fprintf(fp, "# uvs:\n");
  for (auto v : obj.vertices)
    fprintf(fp, "vt %f %f\n", v.t.x, v.t.y);

  fprintf(fp, "# faces:\n");

  const auto &faces = obj.materialFaces[material];

  for (auto i : faces) {
    auto &f = obj.faces[i];
    fprintf(fp, "f");
    for (int i = 0; i < 3; i++) {
      fprintf(fp, " %d/%d/%d", f.ids[i] + 1, f.ids[i] + 1, f.ids[i] + 1);
    }
    fprintf(fp, "\n");
  }

  LOGINFO(__FUNCTION__, "exporting OBJ to file '%s'...", fileName.data());
  LOGINFO(__FUNCTION__, "%s", "done!");
  fclose(fp);
}

}
}
}

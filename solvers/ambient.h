#pragma once

#include "solver.h"

#include "../thirdparty/mtwister/mtwister.h"
#include "../utils/image.h"
#include "../math/noise.h"

namespace mbz {
namespace lightmap {

struct AmbientOcclusionSolver : public Solver {
  AmbientOcclusionSolver(LightmapBuilder &lightmapBuilder)
      :
      Solver(lightmapBuilder) {
    utils::heap::Array<std::unique_ptr<Task>> tasks(lightmapBuilder.heap, 1);
    prepTasks<Task>(tasks);
    std::lock_guard<std::mutex> lg(todoMutex);
    for (int i = 0; i < tasks.size; i++) {
      todo.append_move(std::move(tasks.p()[i]));
    }
  }

  struct Toolbox : public Solver::Toolbox {
    MTRandWrapper mtRand;
    Vector3 skyColor;
    Vector3 randomPointOnSphere() {
      return math::mc::randomPointOnSphere(mtRand);
    }
  };

  virtual Solver::Toolbox* initToolbox(int workerId) override {
    Toolbox *toolbox = new Toolbox();
    toolbox->skyColor = (1.0f / 255.0f) * Vector3(212.0f, 250.0f, 250.0f);
    toolbox->mtRand.seed(2654435761u + 374761393u * uint32_t(workerId));
    return toolbox;
  }

  virtual ~AmbientOcclusionSolver() {
  }

  struct Task : public Solver::Task {
    Vector3 final;
    virtual void perform(utils::multithread::Toolbox *toolbox_) override {
      Toolbox *toolbox = dynamic_cast<Toolbox*>(toolbox_);
      const int N = 80;
      int total = 0;
      float sum = 0.0f;
      while (total < N) {
        auto d = toolbox->randomPointOnSphere();
        if (d.dot(n) <= 0.0f)
          continue;
        total++;
        Ray ray(p + 0.001 * n, d);
        RaySeg raySeg(ray, 10.0f);
        bpcd::Grid::Trace trace(raySeg);
        toolbox->grid->traceRay(raySeg, trace);
        if (!trace.point.has_value())
          sum += d.dot(n);
      }
      sum *= 2.0f / float(N);
      sum = std::clamp(sum, 0.0f, 1.0f);
      final.x = sum * float(c.r) * toolbox->skyColor.x;
      final.y = sum * float(c.g) * toolbox->skyColor.y;
      final.z = sum * float(c.b) * toolbox->skyColor.z;
    }
  };

  void save() {
    int w(lightmapBuilder.get().lightmap->canvas.w), h(lightmapBuilder.get().lightmap->canvas.h);
    utils::img::Image result;
    result.w = w;
    result.h = h;
    result.pixels = std::vector<Color>(w * h);

    {
      std::lock_guard<std::mutex> lg(completedMutex);
      printf("completed size: %d\n", completed.size);
      for (int i = 0; i < completed.size; i++) {
        auto t = static_cast<Task*>(completed.p()[i].release());
        result.pixels[t->y * w + t->x] = Color(t->final.x, t->final.y, t->final.z);
        delete t;
      }
    }
    utils::img::writeImageToPNGFile(result, "ao");
    LOGINFO("AmbientOcclusionSolver::save", "saved result as 'ao.png'");

  }

};


}
}


#pragma once

#include "builder.h"
#include "../utils/workers.h"


namespace mbz {
namespace lightmap {

class Solver : public utils::multithread::Workers<15> {
 public:
  struct Toolbox : public utils::multithread::Toolbox {
    std::shared_ptr<const Lightmap> lightmap = nullptr;
    std::shared_ptr<const math::bpcd::Grid> grid = nullptr;
  };

  struct Task : public utils::multithread::Task {
    int x, y;
    Vector3 p;
    Vector3 n;
    Color c;
  };

  template<typename T>
  void prepTasks(utils::heap::Array<std::unique_ptr<T>> &tasks) {
    static_assert(std::is_base_of<Task, T>::value,
        "T is not a subclass of Task");
    for (int i = 0; i < tasks.size; i++)
      if (tasks.p()[i]) {
        tasks.p()[i].reset();
      }
    tasks.release();

    auto lightmap = lightmapBuilder.get().lightmap;
    auto heap = lightmapBuilder.get().heap;

    int h = lightmap->canvas.h;
    int w = lightmap->canvas.w;

    auto &maskLayer = lightmap->getLayer<0>();
    auto &positionLayer = lightmap->getLayer<1>();
    auto &normalLayer = lightmap->getLayer<2>();
    auto &albedoLayer = lightmap->getLayer<3>();

    tasks.init(heap, w * h);
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int index = lightmap->canvas.xy(x, y);
        if (!maskLayer[index].v)
          continue;
        std::unique_ptr<T> task = std::make_unique<T>();
        task->x = x;
        task->y = y;
        task->p = positionLayer[index].v;
        task->n = normalLayer[index].v;
        task->c = albedoLayer[index].v.sample();
        tasks.append_move(std::move(task));
      }
    }
  }

  std::reference_wrapper<LightmapBuilder> lightmapBuilder;
  virtual Toolbox* initToolbox(int index) = 0;

  virtual std::unique_ptr<utils::multithread::Toolbox> divyToolbox(int workerId) override {
    std::unique_ptr<Toolbox> toolbox = std::unique_ptr<Toolbox>(initToolbox(workerId));

    toolbox->grid = lightmapBuilder.get().grid;
    toolbox->lightmap = lightmapBuilder.get().lightmap;
    return toolbox;
  }

  Solver(std::reference_wrapper<LightmapBuilder> lightmapBuilder_)
      :
      utils::multithread::Workers<15>(lightmapBuilder_.get().heap, 512 * 512, 64),
      lightmapBuilder(lightmapBuilder_) {
  }

  virtual ~Solver() {
  }

};


}
}


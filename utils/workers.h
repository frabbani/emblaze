#pragma once

#include "log.h"
#include "heap.h"
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <string>
#include <string_view>
#include <memory>

#include "array.h"

extern uint64_t getCounter();
extern uint64_t getFreq();

namespace mbz {
namespace utils {
namespace multithread {

struct Toolbox {
  virtual ~Toolbox() = default;
};

struct Task {
  virtual void perform(Toolbox *toolbox) = 0;
  virtual ~Task() = default;
};

template<int N>
class Workers {
  std::array<std::thread, N> workers;

  std::mutex mutex;
  std::condition_variable condition;
  int tasksPer;

 public:
  std::mutex todoMutex;
  std::mutex completedMutex;

  volatile bool start = false;
  utils::heap::Array<std::unique_ptr<Task>> todo;
  utils::heap::Array<std::unique_ptr<Task>> completed;

  virtual std::unique_ptr<Toolbox> divyToolbox(int workerId) {
    return std::make_unique<Toolbox>();
  }
  Workers(std::shared_ptr<heap::Heap> heap, uint32_t taskCount, int tasksPer_ = 1)
      :
      tasksPer(tasksPer_),
      todo(heap, taskCount, heap::Growth::Fixed),
      completed(heap, taskCount, heap::Growth::Fixed) {
    auto work = [&](int id) {
      {
        std::unique_lock<std::mutex> lock(mutex);
        this->condition.wait(lock, [&] {
          return this->start;
        });
      }
      //std::string id;
      //std::stringstream ss;
      //ss << std::this_thread::get_id();
      //ss >> id;
      bool run = true;
      std::vector<std::unique_ptr<Task>> tasks;

      auto toolbox = divyToolbox(id);
      LOGINFO("Workers::work", "worker #%d started...", id);
      while (run) {
        {
          std::lock_guard<std::mutex> lg(todoMutex);
          if (todo.size) {
            for (int i = 0; i < tasksPer; i++) {
              tasks.push_back(std::move(todo.p()[todo.size - 1]));
              todo.size--;
              if (todo.size == 0)
                break;
            }
            LOGINFO("Workers::Workers::work", "grabbed %zu tasks (%d remain)", tasks.size(), todo.size);

          }
          run = todo.size > 0;
        }

        if (!tasks.empty()) {
          for (auto &task : tasks) {
            task->perform(toolbox.get());
          }
          {
            std::lock_guard<std::mutex> lg(completedMutex);
            for (auto &task : tasks)
              completed.append_move(std::move(task));
          }
          tasks.clear();
        }
      }
      LOGINFO("Workers::Workers::work", "worker #%d finished!", id);
    };

    int index = 0;
    for (auto &worker : workers) {
      worker = std::thread([work, index]() {
        work(index);
      });
      index++;
    }
  }

  void join() {
    for (std::thread &worker : workers)
      if (worker.joinable())
        worker.join();
  }

  void begin() {
    std::lock_guard<std::mutex> lock(mutex);
    start = true;
    condition.notify_all();
  }

  void beginJoin() {
    begin();
    join();
  }

  ~Workers() {
    join();
  }

}
;

}
}
}

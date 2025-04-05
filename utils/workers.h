#pragma once

#include "log.h"
#include "heap.h"
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <string>
#include <string_view>
#include "array.h"

extern uint64_t getCounter();
extern uint64_t getFreq();

namespace mbz {
namespace utils {
namespace multithread {



struct Task {
  virtual void perform() = 0;
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

  Workers(std::shared_ptr<heap::Heap> heap, uint32_t taskCount, int tasksPer_ = 1)
      :
      tasksPer(tasksPer_),
      todo(heap, taskCount, heap::Growth::Fixed),
      completed(heap, taskCount, heap::Growth::Fixed) {
    auto work = [&](std::string id) {
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
      LOGINFO("Workers::work", "%s started...", id.c_str());
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
            LOGINFO(id.data(), "(%d from %d) grabbed %zu...", tasksPer, todo.size, tasks.size());

          }
          run = todo.size > 0;
        }

        if (!tasks.empty()) {
          for (auto &task : tasks) {
            task->perform();
          }
          {
            std::lock_guard<std::mutex> lg(completedMutex);
            for (auto &task : tasks)
              completed.append_move(std::move(task));
          }
          tasks.clear();
        }
      }
      LOGINFO("Workers::work", "%s finished!", id.c_str());
    };

    int index = 1;
    for (auto &worker : workers) {
      char tag[16];
      sprintf(tag, "worker %d", index++);
      worker = std::thread([work, tag]() {
        work(tag);
      });
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

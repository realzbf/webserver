#include <pthread.h>

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

#include "sqlconnpool.h"

class ThreadPool {
 public:
  ThreadPool(size_t thread_cnt = 8);
  template <typename T>
  void AddTask(T&& task);
  ThreadPool(ThreadPool&&) = default;
  ~ThreadPool();

 private:
  /* 线程共享结构 */
  struct Pool {
    std::mutex mtx;                           // 互斥访问
    std::condition_variable cond;             // 同步工作
    bool closed;                              // 标记线程池关闭状态
    std::queue<std::function<void()>> tasks;  // 任务队列
  };

  std::shared_ptr<Pool> pool_;
};
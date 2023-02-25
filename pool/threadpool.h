#include <assert.h>
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
  /* 添加新任务，task需要可执行，一般用bind函数生成 */
  void AddTask(T&& task) {
    /* lock_guard在生命周期内一直加锁，结束后自动解锁，即RAII技术，不用担心异常安全问题。
    lock_guard没有提供构造、析构函数以外的接口，如果想要更灵活，可以使用unique_lock
     */
    std::lock_guard<std::mutex> locker(pool_->mtx);
    /* 需要再学习下左右值 */
    pool_->tasks.emplace(std::forward<T>(task));
    /* notify_one只唤醒等待队列中的第一个线程，不存在锁争用，其余线程不会被唤醒
    notify_all唤醒所有等待队列的线程，存在锁争用。
    这里唤醒一个工作线程就可以了 */
    pool_->cond.notify_one();
  }

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
#include <pthread.h>

#include <deque>

#include "sqlpool.h"

template <typename T>
class ThreadPool {
 public:
  ThreadPool(connection_pool db_pool, int __thread_number, int __max_requests);
  ~ThreadPool();

  // 添加新请求
  bool append(T *);

 private:
  // 工作线程
  void *ThreadPool<T>::worker(void *);
  void run();

 private:
  // 线程数
  int thread_number;
  // 最大请求数
  int max_requests;
  bool stop;

  // 请求队列，互斥访问
  std::deque<T *> requests_queue;
  Locker queue_locker;
  Semaphore tasks;

  // 数据库池
  connection_pool *db_pool;
  // 线程数组
  pthread_t *threads;
};
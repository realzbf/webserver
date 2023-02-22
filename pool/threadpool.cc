#include "threadpool.h"

template <typename T>
ThreadPool<T>::ThreadPool(connection_pool __db_pool, int __thread_number,
                          int __max_requests)
    : db_pool(__db_pool),
      thread_number(thread_number),
      max_requests(__max_requests) {
  if (max_requests <= 0 || max_requests <= 0) {
    throw std::exception();
  }
  threads = new pthread_t[thread_number];
  if (!threads) {
    throw std::exception();
  }

  // 给每个线程分配工作
  for (int i = 0; i < thread_number; i++) {
    // 创建失败则销毁线程，抛出异常
    if (pthread_create(threads + i, NULL, worker, this) != 0) {
      delete[] threads;
      throw std::exception();
    }

    // 分离线程，后续不再单独回收
    if (pthread_detach(threads[i])) {
      delete[] threads;
      throw std::exception();
    }
  }
}

template <typename T>
bool ThreadPool<T>::append(T *request) {
  queue_locker.lock();

  // 队列已满
  if (requests_queue.size() >= max_requests) {
    queue_locker.unlock();
    return false;
  }
  requests_queue.push_back(request);
  queue_locker.unlock();

  // 发出信号，请求处理新任务
  tasks.post();
}

template <typename T>
void *ThreadPool<T>::worker(void *arg) {
  ThreadPool *pool = (ThreadPool *)arg;
  pool->run();
  return pool;
}

template <typename T>
void ThreadPool<T>::run() {
  while (!stop) {
    // 等待任务到来
    tasks.wait();

    queue_locker.lock();
    // 任务可能已经完成
    if (requests_queue.empty()) {
      queue_locker.unlock();
      continue;
    }
    T *request = requests_queue.front();
    requests_queue.pop_front();
    queue_locker.unlock();

    if (!request) continue;

    request->mysql = db_pool->getConnection();
    request->process();
    db_pool->releaseConnection(request->mysql);
  }
}
#include "threadpool.h"

#include <iostream>

/* 初始化每一个工作线程 */
ThreadPool::ThreadPool(size_t thread_cnt) : pool_(std::make_shared<Pool>()) {
  assert(thread_cnt > 0);
  // 启动每一个工作线程
  for (size_t i = 0; i < thread_cnt; i++) {
    std::thread([&] {
      std::unique_lock<std::mutex> locker(pool_->mtx);
      while (true) {
        // 如果有新任务
        if (!pool_->tasks.empty()) {
          /* 左值和右值
          左值是表达式结束后依然存在的持久对象（在内存中有确定的位置，可以取地址）
          右值是表达式结束后不再存在的临时对象（在内存中的位置不确定，不可取地址）
          move函数可以将左值转为右值，可以避免拷贝，从而提高性能
          */
          auto task = std::move(pool_->tasks.front());
          pool_->tasks.pop();
          // 取到任务就可以解锁了
          locker.unlock();
          // 执行任务
          task();
          locker.lock();
        } else if (pool_->closed) {  // 线程池被关闭，退出循环
          break;
        } else {  // 任务队列为空，等待新任务
          pool_->cond.wait(locker);
        }
      }
    }).detach();
  }
}

/* 析构函数终止所有线程 */
ThreadPool::~ThreadPool() {
  if (static_cast<bool>(pool_)) {
    std::lock_guard<std::mutex> locker(pool_->mtx);
    pool_->closed = true;
    pool_->cond.notify_all();
  }
}

/* 添加新任务，task需要可执行，一般用bind函数生成 */
template <typename T>
void ThreadPool::AddTask(T&& task) {
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

int main() {
  ThreadPool threadpool;
  for (int j = 0; j < 16; j++) {
    threadpool.AddTask([j = j]() {
      for (int i = 0; i < 10; i++)
        std::cout << "therad " << j << " print " << i << std::endl;
    });
  }
  // 等所有线程退出后进程才结束
  pthread_exit(NULL);
  return 0;
}
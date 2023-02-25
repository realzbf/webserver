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

int main_threadpool() {
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
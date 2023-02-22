#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <stdarg.h>
#include <sys/time.h>

#include <deque>
#include <iostream>
#include <memory>
#include <mutex>

#include "../utils/buffer.h"
#include "blockqueue.h"

using std::string;

class Log {
 public:
  void write(int level, const char* format, ...);
  void init(int level, const char* path = "./log", const char* suffix = ".log",
            int maxQueueCapacity = 1024);
  static Log* Instance();
  bool IsOpen() { return opened_; }
  int GetLevel();
  void flush();

 private:
  int today_;                       // 记录当前天数
  int line_count_;                  // 当前行数
  static const int kMaxLines;       // 最大行数
  static const int kLogNameLength;  // 文件名最大长度
  const char* path_;
  const char* suffix_;
  FILE* fp_;       // 当前写入的文件
  Buffer buffer_;  // 缓冲区
  bool async_;
  std::unique_ptr<BlockDeque<string>> queue_;

 private:
  void AppendLogLevelTitle(int level);
  std::mutex mutex_;
  bool opened_;
};

#define LOG_BASE(level, format, ...)                 \
  do {                                               \
    Log* log = Log::Instance();                      \
    if (log->IsOpen() && log->GetLevel() <= level) { \
      log->write(level, format, ##__VA_ARGS__);      \
      log->flush();                                  \
    }                                                \
  } while (0);

#define LOG_DEBUG(format, ...)         \
  do {                                 \
    LOG_BASE(0, format, ##__VA_ARGS__) \
  } while (0);
#define LOG_INFO(format, ...)          \
  do {                                 \
    LOG_BASE(1, format, ##__VA_ARGS__) \
  } while (0);
#define LOG_WARN(format, ...)          \
  do {                                 \
    LOG_BASE(2, format, ##__VA_ARGS__) \
  } while (0);
#define LOG_ERROR(format, ...)         \
  do {                                 \
    LOG_BASE(3, format, ##__VA_ARGS__) \
  } while (0);

#endif
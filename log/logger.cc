#include "logger.h"

using namespace std;

void Log::write(int level, const char *format, ...) {
  // 获取当前时间
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t tm_sec = now.tv_sec;
  struct tm *sys_time = localtime(&tm_sec);
  char tail[36];
  fill_n(tail, 36, '\0');

  // 时间不是今天或者已经写完一页
  // 写入当前时间 yy_mm_dd
  if (today_ != sys_time->tm_mday || line_count_ % kMaxLines) {
    unique_lock<mutex> locker(mutex_);
    locker.unlock();

    snprintf(tail, 36, "%04d_%02d_%02d", sys_time->tm_year + 1900,
             sys_time->tm_mon + 1, sys_time->tm_mday);
    printf("%s", tail);

    char filename[kLogNameLength];
    fill_n(tail, kLogNameLength, '\0');
    // 时间不是今天
    if (today_ != sys_time->tm_mday) {
      snprintf(filename, kLogNameLength - 72, "%s/%s%s", path_, tail, suffix_);
      // 更新时间
      today_ = sys_time->tm_mday;
      line_count_ = 0;
    } else {  // 新写一页
      snprintf(filename, kLogNameLength - 72, "%s/%s-%d%s", path_, tail,
               (line_count_ / kMaxLines), suffix_);
    }
    locker.lock();
    // 更新文件，互斥访问
    fclose(fp_);
    fp_ = fopen(filename, "a");
    assert(fp_ != nullptr);
  }

  {
    unique_lock<mutex> locker(mutex_);  // 互斥访问队列或者文件
    // 更新行号
    line_count_++;
    // 写入详细时间,精确到毫秒
    int n = snprintf(
        buffer_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
        sys_time->tm_year + 1900, sys_time->tm_mon + 1, sys_time->tm_mday,
        sys_time->tm_hour, sys_time->tm_min, sys_time->tm_sec, now.tv_usec);
    // 更新缓冲区偏移量
    buffer_.HasWritten(n);

    // 写入日志类型
    AppendLogLevelTitle(level);

    va_list vaList;
    va_start(vaList, format);
    int m = vsnprintf(buffer_.BeginWrite(), buffer_.WritableBytes(), format,
                      vaList);
    va_end(vaList);

    buffer_.HasWritten(m);
    buffer_.Append("\n\0", 2);

    if (async_ && queue_ && !queue_->full()) {
      queue_->push_back(buffer_.RetrieveAllToStr());
    } else {
      fputs(buffer_.Peek(), fp_);
    }
    buffer_.RetrieveAll();
    // 离开作用域自动解锁
  }
}

void Log::AppendLogLevelTitle(int level) {
  switch (level) {
    case 0:
      buffer_.Append("[debug]: ", 9);
      break;
    case 1:
      buffer_.Append("[info] : ", 9);
      break;
    case 2:
      buffer_.Append("[warn] : ", 9);
      break;
    case 3:
      buffer_.Append("[error]: ", 9);
      break;
    default:
      buffer_.Append("[info] : ", 9);
      break;
  }
}

int main() {
  Log log;
  log.write(0, "test");
  return 0;
}
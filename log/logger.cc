#include "logger.h"

using namespace std;

Log::Log() {
  line_count_ = 0;
  async_ = false;
  writer_ = nullptr;
  queue_ = nullptr;
  today_ = 0;
  fp_ = nullptr;
}

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
        buffer_.NextWriteable(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
        sys_time->tm_year + 1900, sys_time->tm_mon + 1, sys_time->tm_mday,
        sys_time->tm_hour, sys_time->tm_min, sys_time->tm_sec, now.tv_usec);
    // 更新缓冲区偏移量
    buffer_.MoveWritePos(n);

    // 写入日志类型
    AppendLogLevelTitle(level);

    va_list vaList;
    va_start(vaList, format);
    int m = vsnprintf(buffer_.NextWriteable(), buffer_.GetWritableBytes(), format,
                      vaList);
    va_end(vaList);

    buffer_.MoveWritePos(m);
    buffer_.Append("\n\0", 2);

    if (async_ && queue_ && !queue_->full()) {
      queue_->push_back(buffer_.RetrieveAllToStr());
    } else {
      fputs(buffer_.NextReadable(), fp_);
    }
    buffer_.Reset();
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

void Log::Init(int level = 1, const char *path, const char *suffix,
               int queue_max_size) {
  opened_ = true;
  level_ = level;
  if (queue_max_size > 0) {
    async_ = true;
    if (!queue_) {
      unique_ptr<BlockDeque<std::string>> q(new BlockDeque<std::string>);
      queue_ = move(q);
      std::unique_ptr<std::thread> t(
          new std::thread([] { Log::Instance()->AsyncWrite(); }));
      writer_ = move(t);
    }
  } else {
    async_ = false;
  }

  line_count_ = 0;

  time_t timer = time(nullptr);
  struct tm *sysTime = localtime(&timer);
  struct tm t = *sysTime;
  path_ = path;
  suffix_ = suffix;
  char fileName[kLogNameLength] = {0};
  snprintf(fileName, kLogNameLength - 1, "%s/%04d_%02d_%02d%s", path_,
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
  today_ = t.tm_mday;

  {
    lock_guard<mutex> locker(mutex_);
    buffer_.Reset();
    if (fp_) {
      flush();
      fclose(fp_);
    }

    fp_ = fopen(fileName, "a");
    if (fp_ == nullptr) {
      mkdir(path_, 0777);
      fp_ = fopen(fileName, "a");
    }
    assert(fp_ != nullptr);
  }
}

Log *Log::Instance() {
  static Log instance;
  return &instance;
}

void Log::AsyncWrite() {
  string str = "";
  while (queue_->pop(str)) {
    lock_guard<mutex> locker(mutex_);
    fputs(str.c_str(), fp_);
  }
}

void Log::flush() {
  if (async_) {
    queue_->flush();
  }
  fflush(fp_);
}

int main_logger() {
  Log log;
  log.write(0, "test");
  return 0;
}
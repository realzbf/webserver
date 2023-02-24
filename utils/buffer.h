#ifndef BUFFER_H
#define BUFFER_H

#include <sys/uio.h>

#include <atomic>
#include <string>
#include <vector>

class Buffer {
 public:
  /* 初始化缓冲区大小，默认1024， */
  Buffer(size_t size = 1024) : buffer_(size), read_pos_(0), write_pos_(0) {}
  ~Buffer() = default;

  /* 计算可写空间 */
  inline size_t GetWritableBytes() const {
    return buffer_.size() - write_pos_;
  };

  /* 计算可读空间 */
  inline size_t GetReadableBytes() const { return write_pos_ - read_pos_; };

  /* 获取读指针 */
  inline const char* NextReadable() const { return begin() + read_pos_; }

  /* 获取写指针 */
  inline const char* NextWriteable() const { return begin() + write_pos_; }

  void MoveReadPtr(int offset) {
    assert(offset + read_pos_ <= write_pos_);
    read_pos_ += offset;
  }

  void MoveReadPos(int offset) {
    assert(offset <= GetReadableBytes());
    read_pos_ += offset;
  }

  void MoveReadPtr(const char* target) {
    assert(target - NextReadable() <= GetReadableBytes());
    read_pos_ += (target - NextReadable());
  }

  void Reset() {
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = 0;
    write_pos_ = 0;
  }

 public:
  char* BeginWrite();
  void HasWritten(size_t len);
  void Append(const std::string& str);
  void Append(const char* str, size_t len);
  void Append(const void* data, size_t len);
  void Append(const Buffer& buff);
  std::string RetrieveAllToStr();
  const char* Peek() const;
  void RetrieveAll();

 public:
  ssize_t ReadFd(int fd, int* __errno);

 private:
  /* 获取首指针 */
  inline char* begin() { return &*buffer_.begin(); }
  inline const char* begin() const { return &*buffer_.begin(); }

  std::vector<char> buffer_;
  std::atomic<std::size_t> write_pos_;
  std::atomic<std::size_t> read_pos_;
};

#endif
#ifndef BUFFER_H
#define BUFFER_H

#include <sys/uio.h>

#include <atomic>
#include <string>
#include <vector>

class Buffer {
 public:
  Buffer(size_t size = 1024);
  ~Buffer() = default;
  inline size_t GetWritableBytes() const;
  inline size_t GetReadableBytes() const;

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
  char* BeginPtr() const;
  std::vector<char> buffer_;
  std::atomic<std::size_t> write_pos_;
  std::atomic<std::size_t> read_pos_;
};

#endif
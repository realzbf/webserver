#ifndef BUFFER_H
#define BUFFER_H

#include <sys/uio.h>

#include <atomic>
#include <string>
#include <vector>

class Buffer {
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
  size_t WritableBytes() const;

 public:
  ssize_t ReadFd(int fd, int* __errno);

 private:
  char* BeginPtr() const;
  std::vector<char> buffer_;
  std::atomic<std::size_t> write_pos_;
  std::atomic<std::size_t> read_pos_;
};

#endif
#include "buffer.h"

/* 初始化缓冲区大小，默认1024， */
Buffer::Buffer(size_t size) : buffer_(size), read_pos_(0), write_pos_(0) {}

/* 计算可读空间 */
size_t Buffer::GetReadableBytes() const { return write_pos_ - read_pos_; }

/* 计算可写空间 */
size_t Buffer::GetWritableBytes() const { return buffer_.size() - write_pos_; };

/* 读取用户发来的消息 */
ssize_t Buffer::ReadFd(int fd, int* __errno) {
  char buff[65535];
  struct iovec iov[2];
  const size_t writable = GetWritableBytes();
  /* 分散读， 保证数据全部读完 */
  iov[0].iov_base = BeginPtr() + write_pos_;
  iov[0].iov_len = writable;
  iov[1].iov_base = buff;
  iov[1].iov_len = sizeof(buff);

  const ssize_t len = readv(fd, iov, 2);
  if (len < 0) {
    *__errno = errno;
  } else if (static_cast<size_t>(len) <= writable) {
    write_pos_ += len;
  } else {
    write_pos_ = buffer_.size();
    Append(buff, len - writable);
  }
  return len;
}
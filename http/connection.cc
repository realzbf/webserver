#include "connection.h"

const char* HttpConnection::resources_dir_;
std::atomic<int> HttpConnection::user_count_;
bool HttpConnection::ET;

/* 初始化文件描述符、地址信息、关闭状态 */
HttpConnection::HttpConnection() {
  fd_ = -1;
  addr_ = {0};
  closed_ = true;
}

HttpConnection::~HttpConnection() { Close(); }

void HttpConnection::Init(int fd, const sockaddr_in& addr) {
  assert(fd > 0);
  // 线程安全，无需加锁
  user_count_++;
  addr_ = addr;
  fd_ = fd;
  write_buffer_.Reset();
  read_buffer_.Reset();
  closed_ = false;
  LOG_INFO("Client[%d](%s: %d) connected, users: %d", fd_, GetIp(), GetPort(),
           static_cast<int>(user_count_));
}

/* 获取ip地址，inet_ntoa函数将长整型转为"."分割的ip地址形式
inet_addr将字符串转为无符号长整型
*/
const char* HttpConnection::GetIp() const { return inet_ntoa(addr_.sin_addr); }

/* 获取端口 */
int HttpConnection::GetPort() const { return addr_.sin_port; }

/* 获取套接字 */
int HttpConnection::GetFd() const { return fd_; };

/* 关闭连接 */
void HttpConnection::Close() {}

bool HttpConnection::Process() {
  request_.Init();
  // 没有读取到内容，处理失败
  if (read_buffer_.GetReadableBytes() <= 0) return false;

  // 读取到内容就可以开始解析了
  if (request_.Parse(read_buffer_)) {
    LOG_DEBUG("%s", request_.GetPath().c_str());
    // 解析成功
    response_.Init(resources_dir_, request_.GetPath(), request_.IsKeepAlive(),
                   200);
  } else {
    // 请求内容有误，应返回4xx响应码
    response_.Init(resources_dir_, request_.GetPath(), false, 400);
  }

  // 开始响应
  response_.MakeResponse(write_buffer_);

  // 响应头
  iov_[0].iov_base = const_cast<char*>(write_buffer_.NextReadable());
  iov_[0].iov_len = write_buffer_.GetReadableBytes();
  n_iov_ = 1;

  // 响应内容
  if (response_.GetFileLength() > 0 && response_.GetFile()) {
    iov_[1].iov_base = response_.GetFile();
    iov_[1].iov_len = response_.GetFileLength();
    n_iov_ = 2;
  }

  LOG_DEBUG("Response succeed. Filesize: %d, %d  to %d",
            response_.GetFileLength(), n_iov_, ToWriteBytes());
  return true;
}

/* 读取用户发送的请求内容 */
ssize_t HttpConnection::read(int* __errno) {
  ssize_t sz = -1;
  do {
    sz = read_buffer_.ReadFd(fd_, __errno);
    if (sz <= 0) {
      break;
    }
  } while (ET);
  return sz;
}

ssize_t HttpConnection::write(int* __errno) {
  ssize_t sz = -1;
  /* 1. 写入write_buff
  2. 将write_buff内容转移到客户对应的文件描述符中（本函数做的事）
  */
  do {
    // 聚集写，将iov的所有块写入到fd中，返回写入字节数
    // 需要手动更新iov_base和iov_len
    sz = writev(fd_, iov_, n_iov_);
    // 写入失败
    if (sz <= 0) {
      *__errno = errno;
      break;
    }
    // 无可写入内容，传输结束
    if (iov_[0].iov_len + iov_[1].iov_len == 0) {
      break;
    }
    // 写入字节超出第一块，说明有部分字节写入了第二块
    else if (static_cast<size_t>(sz) > iov_[0].iov_len) {
      // 移动首地址到未写入部分
      iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (sz - iov_[0].iov_len);
      // 更新剩余字节数
      iov_[1].iov_len -= (sz - iov_[0].iov_len);
      // 第二块写完了第一块一定也写完了
      if (iov_[0].iov_len) {
        write_buffer_.Reset();
        iov_[0].iov_len = 0;
      }
    } else {
      // 第一块还没写完，第二块一定还没开始，所以不用管第二块
      iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + sz;
      iov_[0].iov_len -= sz;
      // 更新写入缓冲区的偏移量
      write_buffer_.MoveReadPos(sz);
    }
  } while (ET || ToWriteBytes() > 10240);
  return sz;
}
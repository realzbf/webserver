#include "connection.h"

std::atomic<int> HttpConnection::gUsersNum = 0;

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
  gUsersNum++;
  addr_ = addr;
  fd_ = fd;
  write_buffer_.RetrieveAll();
  read_buffer_.RetrieveAll();
  closed_ = false;
  LOG_INFO("Client[%d](%s: %d) connected, users: %d", fd_, GetIp(), GetPort(),
           static_cast<int>(gUsersNum));
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
  if (request_.parse(read_buffer_)) {
    LOG_DEBUG("%s", request_.path().c_str());
    // 解析成功
    response_.Init(resources_dir, request_.path(), request_.IsKeepAlive(), 200);
  } else {
    // 请求内容有误，应返回4xx响应码
    response_.Init(resources_dir, request_.path(), false, 400);
  }

  // 开始响应
  response_.MakeResponse(write_buffer_);

  // 响应头
  iov_[0].iov_base = const_cast<char*>(write_buffer_.Peek());
  iov_[0].iov_len = write_buffer_.GetReadableBytes();

  // 响应内容
  if (response_.FileLen() > 0 && response_.File()) {
    iov_[1].iov_base = response_.File();
    iov_[1].iov_len = response_.FileLen();
    n_iov_ = 2;
  }

  LOG_DEBUG("Response succeed. Filesize: %d, %d  to %d", response_.FileLen(),
            n_iov_, ToWriteBytes());
  return true;
}

/* read write close待实现 */
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

/* read write process close待实现 */
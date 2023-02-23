#include "webserver.h"

void WebServer::Start() {
  int timeout_ms = -1;  // 阻塞等待

  if (!closed_) {
    LOG_INFO("server started.");
  }

  // 事件监听循环
  while (!closed_) {
    /* 定时器占位
     */

    // 获取时间数
    int n_event = epoller_->Wait(timeout_ms);
    // 处理就绪事件
    for (int i = 0; i < n_event; i++) {
      // 获取事件的fd和type
      int fd = epoller_->GetEventFd(i);
      uint32_t events_type = epoller_->GetEventsType(i);

      // 判断每个类型属于什么类型
      if (fd == listen_fd_) {  // 有新用户连接
        DealListen();
      } else if (events_type &
                 (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {  // 异常事件
        // 处理异常并关闭连接
        DealWithException(&connections_[fd]);
      } else if (events_type & EPOLLIN) {
        // 处理请求
        DealWithRead(&connections_[fd]);
      } else if (events_type & EPOLLOUT) {
        // 处理响应
        DealWithWrite(&connections_[fd]);
      } else {
        ;
      }
    }
  }
}

// void WebServer::DealWithRead(HttpConnection *conn) {
//   // ExtentTime_(client);
//   threadpool_->AddTask(std::bind(&WebServer::Read, this, conn));
// }

// void WebServer::DealWithWrite(HttpConnection *conn) {
//   threadpool_->AddTask(std::bind(&WebServer::Write, this, conn));
// }

void WebServer::DealWithException(HttpConnection *conn) {}

void WebServer::Read(HttpConnection *conn) {
  assert(conn);
  int ret = -1;
  int readErrno = 0;
  ret = conn->read(&readErrno);
  // 没能读取到数据
  if (ret <= 0 && readErrno != EAGAIN) {
    DealWithException(conn);
    return;
  }
  Process(conn);
}

void WebServer::Write(HttpConnection *conn) {
  assert(conn);
  int ret = -1;
  int __errno = 0;
  ret = conn->write(&__errno);
  // 传输完成
  if (conn->ToWriteBytes() == 0) {
    if (conn->IsKeepAlive()) {
      Process(conn);
      return;
    }
  } else if (ret < 0) {
    // 继续传输
    if (__errno == EAGAIN) {
      epoller_->ModFd(conn->GetFd(), event_type_ | EPOLLOUT);
      return;
    }
  }
  CloseConnection(conn);
}

/* 处理用户新请求 */
void WebServer::DealListen() {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  do {
    int fd = accept(listen_fd_, (struct sockaddr *)&addr, &len);
    // 连接失败
    if (fd <= 0) {
      return;
    } else if (HttpConnection::user_count_ >= kMaxFd) {  // 超过最大连接数
      SendError(fd, "Server busy!");
      LOG_WARN("The number of client connections exceeds the limit");
      return;
    }
    AddClient(fd, addr);
  } while (event_type_ && EPOLLET);  // 边缘模式需要一次性处理完
}

/* 给客户发送错误信息 */
void WebServer::SendError(int fd, const char *info) {
  assert(fd > 0);
  int ret = send(fd, info, strlen(info), 0);
  if (ret < 0) {
    LOG_WARN("Failed to send error to client[%d] ", fd);
  }
  close(fd);
}

/* 添加新客户 */
void WebServer::AddClient(int fd, sockaddr_in addr) {
  assert(fd > 0);
  // 初始化
  connections_[fd].Init(fd, addr);
  /*
  定时器占位
  */
  // EPOLLIN指示可读时触发事件，可读意味着客户发来了新请求
  epoller_->AddFd(fd, EPOLLIN | event_type_);
  SetSocketNonBlocking(fd);
  LOG_INFO("Client[%d] connected.", connections_[fd].GetFd());
}

/* 默认情况下socket是blocking的，等待accept、recv、send、connect一系列函数执行完才能返回
可使用fcntl函数将socket设置为非阻塞，这样函数就可以立即返回了，不用等待满足条件才返回，提高并发效率
F_SETFD：设置文件描述词标志
F_GETFD：读取文件描述词标志。
F_GETFL：读取文件状态标志
F_SETFL：设置文件状态标志
这几个参数的区别仍不太了解
*/
void WebServer::SetSocketNonBlocking(int fd) {
  assert(fd > 0);
  // return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
  // 据说这才是正确用法，但两种结果都一样
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}
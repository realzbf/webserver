#include "webserver.h"

WebServer::WebServer(char* base_dir, int port, int trig_mode, int timeout_ms, bool use_linger,
                     int n_thread, bool log, int log_level, int log_queue_size)
    : port_(port),
      timeout_ms_(timeout_ms),
      use_linger_(use_linger),
      closed_(false),
      threadpool_(new ThreadPool(n_thread)),
      epoller_(new Epoller())
       {
  // maybe memory leak
  // resources_dir_ = getcwd(nullptr, 256);

  strcpy(resources_dir_, base_dir);
  strncat(resources_dir_, "/resources/", 16);

  HttpConnection::user_count_ = 0;
  HttpConnection::resources_dir_ = resources_dir_;

  if (log) {
    Log::Instance()->Init(log_level, "./log", ".log", log_queue_size);
  }

  InitEventType(trig_mode);

  if (!InitListenSocket()) {
    closed_ = true;
  }

  if (closed_) {
    LOG_ERROR("========== Server init error!==========");
  } else {
    LOG_INFO("========== Server init ==========");
  }
}

WebServer::~WebServer() {}

void WebServer::InitEventType(int mode) {
  listen_event_type_ = EPOLLRDHUP;
  conn_event_type_ = EPOLLONESHOT | EPOLLRDHUP;
  switch (mode) {
    case 0:
      break;
    case 1:
      conn_event_type_ |= EPOLLET;
      break;
    case 2:
      listen_event_type_ |= EPOLLET;
      break;
    case 3:
      listen_event_type_ |= EPOLLET;
      conn_event_type_ |= EPOLLET;
      break;
    default:
      listen_event_type_ |= EPOLLET;
      conn_event_type_ |= EPOLLET;
      break;
  }
  HttpConnection::ET = (conn_event_type_ & EPOLLET);
}

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
        assert(connections_.count(fd) > 0);
        DealException(&connections_[fd]);
      } else if (events_type & EPOLLIN) {
        // 处理请求
        assert(connections_.count(fd) > 0);
        DealRead(&connections_[fd]);
      } else if (events_type & EPOLLOUT) {
        // 处理响应
        assert(connections_.count(fd) > 0);
        DealWrite(&connections_[fd]);
      } else {
        LOG_ERROR("Unexpected event");
      }
    }
  }
}

void WebServer::CloseConnection(HttpConnection *conn) {
  assert(conn);
  LOG_INFO("Client[%d] quit!", conn->GetFd());
  // 删除对应的文件描述符
  epoller_->DelFd(conn->GetFd());
  // 清理其他连接信息
  conn->Close();
}

void WebServer::DealException(HttpConnection *conn) { CloseConnection(conn); }

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
  } while (listen_event_type_ && EPOLLET);  // 边缘模式需要一次性处理完
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
  epoller_->AddFd(fd, EPOLLIN | conn_event_type_);
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
int WebServer::SetSocketNonBlocking(int fd) {
  assert(fd > 0);
  // return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
  // 据说这才是正确用法，但两种结果都一样
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void WebServer::read(HttpConnection *conn) {
  assert(conn);
  int ret = -1;
  int read_errno = 0;
  ret = conn->read(&read_errno);
  // 没能读取到数据，关闭连接
  if (ret <= 0 && read_errno != EAGAIN) {
    CloseConnection(conn);
    return;
  }
  ModClientFdEvent(conn);
}

void WebServer::write(HttpConnection *conn) {
  assert(conn);
  int ret = -1;
  int write_errno = 0;
  ret = conn->write(&write_errno);
  // 传输完成
  if (conn->ToWriteBytes() == 0) {
    if (conn->IsKeepAlive()) {
      ModClientFdEvent(conn);
      return;
    }
  } else if (ret < 0) {
    // 继续传输
    if (write_errno == EAGAIN) {
      epoller_->ModFd(conn->GetFd(), conn_event_type_ | EPOLLOUT);
      return;
    }
  }
  CloseConnection(conn);
}

void WebServer::DealRead(HttpConnection *conn) {
  /* 定时器占位
   */
  threadpool_->AddTask(std::bind(&WebServer::read, this, conn));
}

void WebServer::DealWrite(HttpConnection *conn) {
  /* 定时器占位
   */
  threadpool_->AddTask(std::bind(&WebServer::write, this, conn));
}

bool WebServer::InitListenSocket() {
  int ret;
  struct sockaddr_in addr;

  if (port_ > 65535 || port_ < 1024) {
    LOG_ERROR("Port:%d error!", port_);
    return false;
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port_);

  struct linger optLinger = {0};

  if (use_linger_) {
    /* 优雅关闭: 直到所剩数据发送完毕或超时 */
    optLinger.l_onoff = 1;
    optLinger.l_linger = 1;
  }

  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    LOG_ERROR("Create socket error!", port_);
    return false;
  }

  ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &optLinger,
                   sizeof(optLinger));
  if (ret < 0) {
    close(listen_fd_);
    LOG_ERROR("Init linger error!", port_);
    return false;
  }

  int optval = 1;
  /* 端口复用 */
  /* 只有最后一个套接字会正常接收数据。 */
  ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                   sizeof(int));
  if (ret == -1) {
    LOG_ERROR("set socket setsockopt error !");
    close(listen_fd_);
    return false;
  }

  ret = bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0) {
    LOG_ERROR("Bind Port:%d error!", port_);
    close(listen_fd_);
    return false;
  }

  ret = listen(listen_fd_, 6);
  if (ret < 0) {
    LOG_ERROR("Listen port:%d error!", port_);
    close(listen_fd_);
    return false;
  }
  ret = epoller_->AddFd(listen_fd_, listen_event_type_ | EPOLLIN);
  if (ret == 0) {
    LOG_ERROR("Add listen error!");
    close(listen_fd_);
    return false;
  }
  SetSocketNonBlocking(listen_fd_);
  LOG_INFO("Server port:%d", port_);
  return true;
}

/* 修改客户描述符事件类型 */
void WebServer::ModClientFdEvent(HttpConnection *conn) {
  if (conn->Process()) {
    epoller_->ModFd(conn->GetFd(), conn_event_type_ | EPOLLOUT);
  } else {
    epoller_->ModFd(conn->GetFd(), conn_event_type_ | EPOLLIN);
  }
}
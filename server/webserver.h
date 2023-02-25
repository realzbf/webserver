#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <fcntl.h>

#include <functional>
#include <unordered_map>

#include "../http/connection.h"
#include "../log/logger.h"
#include "../pool/threadpool.h"
#include "../utils/epoller.h"

class WebServer {
 private:
  bool closed_;
  bool use_linger_;
  int listen_fd_;
  static const int kMaxFd = 65536;
  int port_;
  int timeout_ms_;
  char *resources_dir_;

 private:
  uint32_t listen_event_type_, conn_event_type_;
  std::unique_ptr<Epoller> epoller_;
  std::unique_ptr<ThreadPool> threadpool_;
  std::unordered_map<int, HttpConnection> connections_;

 private:
  void DealRead(HttpConnection *conn);
  void read(HttpConnection *conn);
  void write(HttpConnection *conn);
  void DealWrite(HttpConnection *conn);
  void DealException(HttpConnection *conn);
  void CloseConnection(HttpConnection *conn);
  void SendError(int fd, const char *info);
  void AddClient(int fd, sockaddr_in addr);
  void ModClientFdEvent(HttpConnection *conn);
  void DealListen();
  void InitEventType(int mode);
  int SetSocketNonBlocking(int fd);
  bool InitListenSocket();

 public:
  WebServer(int port, int trig_mode, int timeout_ms, bool use_linger_,
            int n_thread, bool log, int log_level, int log_queue_size);

  ~WebServer();
  void Start();
};

#endif
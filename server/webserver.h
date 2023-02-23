#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <functional>
#include <unordered_map>

#include "../http/connection.h"
#include "../log/logger.h"
#include "../pool/threadpool.h"
#include "../utils/epoller.h"

class WebServer {
 private:
  bool closed_;
  int listen_fd_;
  static const int kMaxFd = 65536;

 private:
  uint32_t event_type_;
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
  void SetSocketNonBlocking(int fd);

 public:
  WebServer();
  void Start();
};

#endif
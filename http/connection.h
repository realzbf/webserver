#include <arpa/inet.h>
#include <errno.h>
#include <mysql/mysql.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <atomic>
#include <regex>
#include <string>
#include <unordered_map>

#include "../log/logger.h"
#include "../utils/buffer.h"
#include "request.h"
#include "response.h"

using std::string;
using std::unordered_map;

class HttpConnection {
 private:
  int fd_;
  struct sockaddr_in addr_;
  bool closed_;
  struct iovec iov_[2];
  int n_iov_;
  // 读写缓冲区
  Buffer read_buffer_;
  Buffer write_buffer_;
  HttpRequest request_;
  HttpResponse response_;

 public:
  HttpConnection();
  ~HttpConnection();
  void Init(int fd, const sockaddr_in &addr);
  bool Process();
  ssize_t read(int *__errno);
  ssize_t write(int *__errno);
  static bool ET;
  static const char *resources_dir_;
  static std::atomic<int> user_count_;

 public:
  void Close();

 public:
  const char *GetIp() const;
  int GetPort() const;
  int GetFd() const;
  inline int ToWriteBytes() { return iov_[0].iov_len + iov_[1].iov_len; }
  inline bool IsKeepAlive() const { return request_.IsKeepAlive(); }
};
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

using std::string;
using std::unordered_map;

class HttpConnection {
 private:
  int fd_;
  struct sockaddr_in addr_;
  bool closed_;
  // 读写缓冲区
  Buffer read_buffer_;
  Buffer write_buffer_;

 public:
  HttpConnection();
  ~HttpConnection();
  void Init(int fd, const sockaddr_in &addr);
  bool Process();
  ssize_t read(int *__errno);
  ssize_t write(int *__errno);
  static std::atomic<int> gUsersNum;

 public:
  void Close();

 public:
  const char *GetIp() const;
  int GetPort() const;
  int GetFd() const;
};
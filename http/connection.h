#include <errno.h>
#include <mysql/mysql.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <atomic>
#include <regex>
#include <string>
#include <unordered_map>

#include "../utils/buffer.h"

using std::string;
using std::unordered_map;

class HttpConnection {
 public:
  enum ParseState { kRequestLine, kHeaders, kBody, kFinish };
  ssize_t read(int *__errno);
  ssize_t write(int *__errno);
  int ToWriteBytes() { return iov_[0].iov_len + iov_[1].iov_len; }
  bool IsKeepAlive();
  int GetFd();

 private:
  bool ParseRequestLine(const string &line);
  void ParseHeader(const string &line);
  void ParseBody(const string &line);
  void ParsePost();

 public:
  static const int REQUEST_BUFFER_SIZE = 2048;
  static const int RESPONSE_BUFFER_SIZE = 1024;

  HttpConnection();
  ~HttpConnection();

 public:
  void init(int __fd, sockaddr_in __addr);
  MYSQL *getConnection();
  void releaseConnection(MYSQL *);
  // 读取用户数据，直到无数据或者对方关闭连接
  bool read_once();

 private:
  int fd_;
  sockaddr_in addr;

  // 请求报文和响应报文
  char request_buffer[REQUEST_BUFFER_SIZE];
  int read_index;
  char response_buffer[RESPONSE_BUFFER_SIZE];

 private:
  string method_;
  string path_;
  string version_;
  string body_;
  unordered_map<string, string> header_;
  string state_;
  Buffer in_buffer_;
  Buffer out_buffer_;
  struct iovec iov_[2];

 public:
  static std::atomic<int> user_count_;
};
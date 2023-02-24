#ifndef REQUEST_H
#define REQUEST_H

#include <errno.h>
#include <mysql/mysql.h>  //mysql

#include <algorithm>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../log/logger.h"
#include "../pool/sqlconn.h"
#include "../pool/sqlconnpool.h"
#include "../utils/buffer.h"

using std::string;
using std::unordered_map;
using std::unordered_set;

/* HTTP请求结构(以\r\n作为结束字符)
1. 请求行：
  请求类型、资源路径、HTTP版本
2. 请求头部：
  host：资源所在服务器域名
  User-Agent：客户端信息
  Accept：用户代理可处理的媒体类型
  Accept-Encoding：用户代理可处理的内容编码
  Accept-Language：用户代理可处理的语言集
  Content-Type：实现主体的媒体类型
  Content-Length：实现主体的大小
  Connection：连接管理，可以是Keep-Alive（后续请求无需重新建立连接，但会占用服务器资源）或close
3. 空行
3. 请求数据（主体）
*/
class HttpRequest {
 public:
  enum PARSE_STATE {
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH,
  };

  enum HTTP_CODE {
    NO_REQUEST = 0,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURSE,
    FORBIDDENT_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION,
  };

  HttpRequest() { Init(); }
  ~HttpRequest() = default;

  void Init();
  bool Parse(Buffer& buff);

  inline string& GetPath() { return path_; };
  inline const string GetPath() const { return path_; };
  inline const bool IsKeepAlive() const;

 private:
  bool ParseRequestLine(const string& line);
  void ParseHeader(const string& line);
  void ParseBody(const string& line);

  // 解析状态
  PARSE_STATE state_;
  // 请求类型、资源路径、HTTP版本、主体
  string method_, path_, version_, body_;
  // 请求头
  unordered_map<string, string> header_;
  // post内容
  unordered_map<string, string> post_;
  // 可访问的资源路径
  static const unordered_set<string> kAvaiableHtml;
  static const unordered_map<string, int> DEFAULT_HTML_TAG;
};

#endif
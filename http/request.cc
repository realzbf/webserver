#include "request.h"

/* 可访问的资源路径 */
const unordered_set<string> HttpRequest::kAvaiableHtml{
    "/index", "/register", "/login", "/welcome", "/video", "/picture"};

/* 初始化各个参数 */
void HttpRequest::Init() {
  method_ = path_ = version_ = body_ = "";
  state_ = REQUEST_LINE;
  header_.clear();
  post_.clear();
}

/* 解析请求 */
bool HttpRequest::Parse(Buffer& buff) {
  static const string kCrlf = "\r\n";
  // 无可读内容
  if (buff.GetReadableBytes() < 0) return false;

  // 有限状态机
  while (buff.GetReadableBytes() > 0 && state_ != FINISH) {
    // 未读的区间是read_pos到write_pos这一段
    const char* line_end = std::search(
        buff.NextReadable(), buff.NextWriteableConst(), kCrlf.cbegin(), kCrlf.cend());
    // 取一行
    string line(buff.NextReadable(), line_end);
    // 根据当前状态匹配解析
    switch (state_) {
      case REQUEST_LINE:
        // 请求行有错误
        if (!ParseRequestLine(line)) return false;
        break;
      case HEADERS:
        ParseHeader(line);
        // 可能没有body，可以直接结束
        if (buff.GetReadableBytes() <= 2) state_ = FINISH;
        break;
      case BODY:
        ParseBody(line);
        break;
      default:
        break;
    }
    if (line_end == buff.NextWriteable()) break;
    buff.MoveReadPtr(line_end + 2);
  }

  LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(),
            version_.c_str());
  return true;
}

/* 解析请求行 */
bool HttpRequest::ParseRequestLine(const string& line) {
  std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch subMatch;
  /* 取出请求类型、资源路径、HTTP版本*/
  if (!regex_match(line, subMatch, patten)) return false;

  method_ = subMatch[1];
  path_ = subMatch[2];
  version_ = subMatch[3];

  LOG_ERROR("RequestLine Error");

  /* 解析路径， 加上html后缀 */
  if (path_ == "/") {  // 根目录默认为index.html
    path_ = "/index.html";
  } else if (kAvaiableHtml.find(path_) != kAvaiableHtml.end()) {
    path_ += ".html";
  } else
    return false;  // 无效资源路径

  state_ = HEADERS;
  return true;
}

/* 解析请求头 */
void HttpRequest::ParseHeader(const string& line) {
  std::regex patten("^([^:]*): ?(.*)$");
  std::smatch subMatch;
  if (regex_match(line, subMatch, patten)) {
    header_[subMatch[1]] = subMatch[2];
  } else {
    state_ = BODY;
  }
}

/* 判断是否持久连接 */
bool HttpRequest::IsKeepAlive() const {
  if (header_.count("Connection") > 0) {
    return header_.find("Connection")->second == "keep-alive" &&
           version_ == "1.1";
  }
  return false;
}

/* 解析主体 */
void HttpRequest::ParseBody(const string& line) {
  body_ = line;
  // 用户提交了表单，在本例是登录
  if (method_ == "POST" &&
      header_["Content-Type"] == "application/x-www-form-urlencoded") {
    /* 暂不作校验，直接返回welcome页面 */
    path_ = "/welcome.html";
  }
  state_ = FINISH;
  LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}
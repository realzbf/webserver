#include "response.h"

const unordered_map<string, string> HttpResponse::kSuffixToType = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

const unordered_map<int, string> HttpResponse::kCodeToStatus = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const unordered_map<int, string> HttpResponse::kErrorCodeToPath = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse() {
  code_ = -1;
  path_ = "";
  resources_dir_ = "";
  keep_alive_ = false;
  mmap_ptr = nullptr;
  file_stat_ = {0};
}

/* 虚构函数只需要取消映射文件，没有其他资源要释放 */
HttpResponse::~HttpResponse() { UnmapFile(); }

/* 初始化各参数 */
void HttpResponse::Init(const string& resources_dir, string& path,
                        bool keep_alive, int code) {
  assert(resources_dir != "");
  if (mmap_ptr) {
    UnmapFile();
  }
  code_ = code;
  keep_alive_ = keep_alive;
  path_ = path;
  resources_dir_ = resources_dir;
  mmap_ptr = nullptr;
  file_stat_ = {0};
}

/* 生成响应内容 */
void HttpResponse::MakeResponse(Buffer& buff) {
  /* 1.
  string有两个函数用于获取C风格的字符串：c_str()和data()，在c11之前，前者可以指向末位不为\0的字符串，在c11之后两者无区别
  2. stat函数用于获取文件信息
  3. S_ISDIR判断是否为目录
  这行代码主要是判断文件是否有效，无效即返回404
  */
  if (stat((resources_dir_ + path_).data(), &file_stat_) < 0 ||
      S_ISDIR(file_stat_.st_mode)) {
    code_ = 404;
  }
  /* 判断文件权限
  S_IRUSR：用户读权限
  S_IWUSR：用户写权限
  S_IRGRP：用户组读权限
  S_IWGRP：用户组写权限
  S_IROTH：其他组读权限
  S_IWOTH：其他组写权限
  用户无权访问返回403
  */
  else if (!(file_stat_.st_mode & S_IROTH)) {
    code_ = 403;
  } else if (code_ == -1) {
    //  可访问该资源
    code_ = 200;
  }

  // 4xx 错误
  if (kErrorCodeToPath.count(code_) > 0) {
    SetErrorHtml();
  }
  SetStateLine(buff);
  SetHeader(buff);
  SetContent(buff);
}

/* 设置错误页面路径 */
void HttpResponse::SetErrorHtml() {
  path_ = kErrorCodeToPath.find(code_)->second;
  stat((resources_dir_ + path_).data(), &file_stat_);
}

/* 设置错误页面 */
void HttpResponse::SetErrorContent(Buffer& buff, string message) {
  string body;
  string status;
  body += "<html><title>Error</title>";
  body += "<body bgcolor=\"ffffff\">";
  if (kCodeToStatus.count(code_) > 0) {
    status = kCodeToStatus.find(code_)->second;
  } else {
    status = "Bad Request";
  }
  body += std::to_string(code_) + " : " + status + "\n";
  body += "<p>" + message + "</p>";
  body += "<hr><em>TinyWebServer</em></body></html>";

  buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
  buff.Append(body);
}

/* 设置状态行 */
void HttpResponse::SetStateLine(Buffer& buff) {
  // 需能够处理所有响应码
  assert(kCodeToStatus.count(code_) > 0);
  string status = kCodeToStatus.find(code_)->second;
  // 状态行
  buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

/* 设置响应头 */
void HttpResponse::SetHeader(Buffer& buff) {
  buff.Append("Connection: ");
  // if (keep_alive_) {
  //   buff.Append("keep-alive\r\n");
  //   buff.Append("keep-alive: max=6, timeout=120\r\n");
  // } else {
  //   buff.Append("close\r\n");
  // }
  buff.Append("close\r\n");
  buff.Append("Content-type: " + GetFileType() + "\r\n");
}

/* 设置响应内容 */
void HttpResponse::SetContent(Buffer& buff) {
  // 只读模式打开文件
  int fp = open((resources_dir_ + path_).data(), O_RDONLY);
  if (fp < 0) {
    SetErrorContent(buff, "File NotFound!");
    return;
  }

  /* mmap将文件映射到内存提高文件的访问速度，0指示系统自动选取首地址，PROT_READ表示可读取，MAP_PRIVATE建立一个写入时拷贝的私有映射，对该区域的任何修改都不会写回原来的文件内容
   */
  LOG_DEBUG("file path %s", (resources_dir_ + path_).data());
  int* ret = (int*)mmap(0, file_stat_.st_size, PROT_READ, MAP_PRIVATE, fp, 0);
  if (*ret == -1) {
    // 映射失败
    SetErrorContent(buff, "File Not Found!");
    return;
  }
  mmap_ptr = (char*)ret;
  // 要记得关闭文件
  close(fp);
  buff.Append("Content-length: " + std::to_string(file_stat_.st_size) +
              "\r\n\r\n");
}

/* 取消映射文件 */
void HttpResponse::UnmapFile() {
  if (mmap_ptr) {
    munmap(mmap_ptr, file_stat_.st_size);
    mmap_ptr = nullptr;
  }
}

/* 获取文件类型 */
string HttpResponse::GetFileType() {
  // 判断文件类型
  string::size_type idx = path_.find_last_of('.');
  // 没有后缀，默认为纯文本
  if (idx == string::npos) {
    return "text/plain";
  }
  // 能找到该类型
  string suffix = path_.substr(idx);
  if (kSuffixToType.count(suffix) == 1) {
    return kSuffixToType.find(suffix)->second;
  }
  // 其他默认为纯文本
  return "text/plain";
}
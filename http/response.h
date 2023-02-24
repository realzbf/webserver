#ifndef RESPONSE_H
#define RESPONSE_H

#include <fcntl.h>     // open
#include <sys/mman.h>  // mmap, munmap
#include <sys/stat.h>  // stat
#include <unistd.h>    // close

#include <unordered_map>

#include "../log/logger.h"
#include "../utils/buffer.h"

using std::string;
using std::unordered_map;
class HttpResponse {
 public:
  HttpResponse();
  ~HttpResponse();

  void Init(const std::string& srcDir, std::string& path,
            bool isKeepAlive = false, int code = -1);
  void MakeResponse(Buffer& buff);
  void UnmapFile();

  // 获取状态码
  inline int GetCode() const { return code_; }

  // 获取文件映射后的内存指针
  inline char* GetFile() { return mmap_ptr; };

  // 获取文件长度
  inline size_t GetFileLength() const { return file_stat_.st_size; }

 private:
  void SetErrorHtml();
  void SetStateLine(Buffer& buff);
  void SetHeader(Buffer& buff);
  void SetContent(Buffer& buff);
  void SetErrorContent(Buffer& buff, string message);
  std::string GetFileType();

 private:
  int code_;
  bool keep_alive_;

  std::string path_;
  std::string resources_dir_;

  char* mmap_ptr;
  struct stat file_stat_;

  static const std::unordered_map<std::string, std::string> kSuffixToType;
  static const std::unordered_map<int, std::string> kCodeToStatus;
  static const std::unordered_map<int, std::string> kErrorCodeToPath;
};

#endif
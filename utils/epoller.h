#ifndef EPOLLER_H
#define EPOLLER_H

#include <assert.h>  // close()
#include <errno.h>
#include <fcntl.h>      // fcntl()
#include <sys/epoll.h>  //epoll_ctl()
#include <unistd.h>     // close()

#include <vector>

class Epoller {
 public:
  explicit Epoller(int maxEvent = 1024);

  ~Epoller();

  bool AddFd(int fd, uint32_t events);

  bool ModFd(int fd, uint32_t events);

  bool DelFd(int fd);

  int Wait(int timeoutMs = -1);

  int GetEventFd(size_t i) const;

  uint32_t GetEventsType(size_t i) const;

 private:
  // 内核事件表
  int epfd_;
  /* 存储从内核拷贝过来的事件
  epoll_event由events和data组成，其中events成员描述时间类型，data成员用于存储用户数据
  data.fd指定事件所从属的目标文件描述符
  */

  std::vector<struct epoll_event> events_;
};

#endif  // EPOLLER_H
#include "epoller.h"

#include <iostream>

Epoller::Epoller(int maxEvent) : epfd_(epoll_create(512)), events_(maxEvent) {
  assert(epfd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller() { close(epfd_); }

/*
查找就绪事件，timeout为-1时一般为阻塞等待，为0时非阻塞等待，即立即返回结果
返回0表示超时，返回-1表示出错，并设置errno
*/
int Epoller::Wait(int timeout) {
  return epoll_wait(epfd_, &events_[0], static_cast<int>(events_.size()),
                    timeout);
}

/* 获取对应索引的时间描述符fd */
int Epoller::GetEventFd(size_t i) const {
  assert(i < events_.size() && i >= 0);
  return events_[i].data.fd;
}

/* 获取对应索引的时间类型events */
uint32_t Epoller::GetEventsType(size_t i) const {
  assert(i < events_.size() && i >= 0);
  return events_[i].events;
}

/* 注册事件 */
bool Epoller::AddFd(int fd, uint32_t events) {
  if (fd < 0) return false;
  epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = events;
  int ret = epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
  std::cout << errno << std::endl;
  return 0 == ret;
}

/* 删除事件 */
bool Epoller::DelFd(int fd) {
  if (fd < 0) return false;
  epoll_event ev = {0};
  return 0 == epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &ev);
}

/* 修改事件 */
bool Epoller::ModFd(int fd, uint32_t events) {
  if (fd < 0) return false;
  epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = events;
  return 0 == epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

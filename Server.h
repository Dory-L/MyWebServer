#pragma once
#include <memory>
#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

class Server {
 public:
  Server(EventLoop *loop, int threadNum, int port);
  ~Server() {}
  EventLoop *getLoop() const { return loop_; }
  void start();
  void handNewConn();
  void handThisConn() { loop_->updatePoller(acceptChannel_); }

 private:
  EventLoop *loop_;
  int threadNum_;//线程数量
  std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
  bool started_;//开始标志
  std::shared_ptr<Channel> acceptChannel_;//接受连接事件
  int port_;//端口号
  int listenFd_;//监听fd
  static const int MAXFDS = 100000;
};
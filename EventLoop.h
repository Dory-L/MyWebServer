// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "Channel.h"
#include "Epoll.h"
#include "Util.h"
#include "base/CurrentThread.h"
#include "base/Logging.h"
#include "base/Thread.h"

#include <iostream>
using namespace std;

class EventLoop
{
public:
    typedef std::function<void()> Functor;
    EventLoop();
    ~EventLoop();
    void loop();
    void quit();
    void runInLoop(Functor &&cb);
    void queueInLoop(Functor &&cb);
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
    void assertInLoopThread() { assert(isInLoopThread()); }
    //全双工关闭channel的fd上的连接
    void shutdown(shared_ptr<Channel> channel) { shutDownWR(channel->getFd()); }
    void removeFromPoller(shared_ptr<Channel> channel)
    {
        // shutDownWR(channel->getFd());
        poller_->epoll_del(channel);
    }
    void updatePoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        poller_->epoll_mod(channel, timeout);
    }
    //添加事件到epollfd
    void addToPoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        poller_->epoll_add(channel, timeout);
    }

private:
    // 声明顺序 wakeupFd_ > pwakeupChannel_
    bool looping_;
    shared_ptr<Epoll> poller_;
    int wakeupFd_;      //eventfd
    bool quit_;          //退出标志
    bool eventHandling_; //正在处理事件标志
    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_; //挂起函数队列
    bool callingPendingFunctors_;          //调用挂起函数标志
    const pid_t threadId_;
    shared_ptr<Channel> pwakeupChannel_; //唤醒loop线程事件

    void wakeup();
    void handleRead();
    void doPendingFunctors();
    void handleConn();
};

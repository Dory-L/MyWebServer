#pragma once
#include <memory>
#include <vector>
#include "EventLoopThread.h"
#include "base/Logging.h"
#include "base/noncopyable.h"

class EventLoopThreadPool : noncopyable
{
public:
    EventLoopThreadPool(EventLoop *baseLoop, int numThreads);

    ~EventLoopThreadPool() { LOG << "~EventLoopThreadPool()"; }
    void start();

    EventLoop *getNextLoop();

private:
    EventLoop *baseLoop_;//基础事件循环
    bool started_;//启动标志
    int numThreads_;//线程池线程数量
    int next_;//下一个线程编号
    std::vector<std::shared_ptr<EventLoopThread>> threads_;//线程队列
    std::vector<EventLoop *> loops_;//eventloop队列
};
#pragma once
#include "EventLoop.h"
#include "base/Condition.h"
#include "base/MutexLock.h"
#include "base/Thread.h"
#include "base/noncopyable.h"

class EventLoopThread : noncopyable
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop *startLoop();

private:
    void threadFunc();
    EventLoop *loop_;
    bool exiting_;//退出标志
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};
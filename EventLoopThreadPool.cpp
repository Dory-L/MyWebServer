#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, int numThreads)
    : baseLoop_(baseLoop), started_(false), numThreads_(numThreads), next_(0)
{
    if (numThreads_ <= 0)
    {
        LOG << "numThreads_ <= 0";
        abort();
    }
}

//启动线程池
void EventLoopThreadPool::start()
{
    baseLoop_->assertInLoopThread();
    started_ = true;
    for (int i = 0; i < numThreads_; ++i)
    {
        std::shared_ptr<EventLoopThread> t(new EventLoopThread());
        threads_.push_back(t);
        loops_.push_back(t->startLoop());
    }
}

//获取下一个loop
//valid after calling start()
//rount-robin
EventLoop *EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    assert(started_);//线程池已启动
    EventLoop *loop = baseLoop_;
    if (!loops_.empty())
    {
        //round-robin
        loop = loops_[next_];
        next_ = (next_ + 1) % numThreads_;
    }
    return loop;
}
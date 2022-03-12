// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoop.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <iostream>
#include "Util.h"
#include "base/Logging.h"

using namespace std;

__thread EventLoop *t_loopInThisThread = 0;

//创建eventfd
int createEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      poller_(new Epoll()),
      wakeupFd_(createEventfd()),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      pwakeupChannel_(new Channel(this, wakeupFd_))
{
    if (t_loopInThisThread)
    {
        // LOG << "Another EventLoop " << t_loopInThisThread << " exists in this
        // thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    // pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
    pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead, this));
    pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
    poller_->epoll_add(pwakeupChannel_, 0);
}

void EventLoop::handleConn()
{
    // poller_->epoll_mod(wakeupFd_, pwakeupChannel_, (EPOLLIN | EPOLLET |
    // EPOLLONESHOT), 0);
    updatePoller(pwakeupChannel_, 0);
}

EventLoop::~EventLoop()
{
    // wakeupChannel_->disableAll();
    // wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread = NULL;
}

//用一个数据唤醒loop线程
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, (char *)(&one), sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

//处理读事件
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    // pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);//边沿触发
}

//让loop线程运行cb函数
void EventLoop::runInLoop(Functor &&cb)
{
    if (isInLoopThread())//如果是loop线则直接运行
        cb();
    else
        queueInLoop(std::move(cb));//如果不是则添加到loop的线程中
}

//将cb函数添加到挂起函数队列中，等待loop线程执行
void EventLoop::queueInLoop(Functor &&cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
    //该线程不是loop线程或者本线程是loop线程且正在调用挂起线程，唤醒loop线程
    if (!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}

//事件循环
void EventLoop::loop()
{
    assert(!looping_);
    assert(isInLoopThread());//在事件循环的线程中
    looping_ = true;//循环开始
    quit_ = false;
    // LOG_TRACE << "EventLoop " << this << " start looping";
    std::vector<SP_Channel> ret;//活跃事件
    while (!quit_)
    {
        // cout << "doing" << endl;
        ret.clear();
        ret = poller_->poll();//通过epoll得到活跃事件
        eventHandling_ = true;
        for (auto &it : ret)
            it->handleEvents();//处理事件
        eventHandling_ = false;
        doPendingFunctors();//调用挂起函数
        poller_->handleExpired();
    }
    looping_ = false;
}

//处理挂起函数
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)//逐个执行挂起函数
        functors[i]();
    callingPendingFunctors_ = false;//挂起函数调用完毕
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())//如果当前线程不是事件循环线程
    {
        wakeup();
    }
}
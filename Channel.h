/*Channel是Reactor结构中的“事件”，它自始至终都属于一个EventLoop，负责一个文件描述符的IO事件，
在Channel类中保存这IO事件的类型以及对应的回调函数，当IO事件发生时，最终会调用到Channel类中的回调函数。
因此，程序中所有带有读写时间的对象都会和一个Channel关联，包括loop中的eventfd，listenfd，HttpData等。*/
#pragma once
#include <sys/epoll.h>
#include <sys/epoll.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "Timer.h"

class EventLoop;
class HttpData;

class Channel
{
private:
    typedef std::function<void()> CallBack;
    EventLoop *loop_;
    int fd_;
    __uint32_t events_;
    __uint32_t revents_;
    __uint32_t lastEvents_;

    // 方便找到上层持有该Channel的对象
    std::weak_ptr<HttpData> holder_;

private:
    int parse_URI();
    int parse_Headers();
    int analysisRequest();

    CallBack readHandler_;//读回调
    CallBack writeHandler_;//写回调
    CallBack errorHandler_;//错误回调
    CallBack connHandler_;//连接回调

public:
    Channel(EventLoop *loop);
    Channel(EventLoop *loop, int fd);
    ~Channel();
    int getFd();
    void setFd(int fd);

    void setHolder(std::shared_ptr<HttpData> holder) { holder_ = holder; }
    //返回Channel的HttpData持有者
    std::shared_ptr<HttpData> getHolder()
    {
        std::shared_ptr<HttpData> ret(holder_.lock());
        return ret;
    }

    void setReadHandler(CallBack &&readHandler) { readHandler_ = readHandler; }
    void setWriteHandler(CallBack &&writeHandler)
    {
        writeHandler_ = writeHandler;
    }
    void setErrorHandler(CallBack &&errorHandler)
    {
        errorHandler_ = errorHandler;
    }
    void setConnHandler(CallBack &&connHandler) { connHandler_ = connHandler; }

    void handleEvents()
    {
        events_ = 0;
        if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
        {
            events_ = 0;
            return;
        }
        if (revents_ & EPOLLERR)
        {
            if (errorHandler_)
                errorHandler_();
            events_ = 0;
            return;
        }
        if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
        {
            handleRead();
        }
        if (revents_ & EPOLLOUT)
        {
            handleWrite();
        }
        handleConn();
    }
    void handleRead();
    void handleWrite();
    void handleError(int fd, int err_num, std::string short_msg);
    void handleConn();

    void setRevents(__uint32_t ev) { revents_ = ev; }

    void setEvents(__uint32_t ev) { events_ = ev; }
    __uint32_t &getEvents() { return events_; }

    bool EqualAndUpdateLastEvents()
    {
        bool ret = (lastEvents_ == events_);
        lastEvents_ = events_;
        return ret;
    }

    __uint32_t getLastEvents() { return lastEvents_; }
};

typedef std::shared_ptr<Channel> SP_Channel;
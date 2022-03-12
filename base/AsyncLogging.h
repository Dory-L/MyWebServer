//AsyncLogging是核心，它负责启动一个log线程，专门用来将log写入LogFile，应用了“双缓冲技术”，其实有4个以上的缓冲区，但思想是一样的。
//AsyncLogging负责(定时到或被填满时)将缓冲区中的数据写入LogFile中。
#pragma once
#include <functional>
#include <string>
#include <vector>
#include "CountDownLatch.h"
#include "LogStream.h"
#include "MutexLock.h"
#include "Thread.h"
#include "noncopyable.h"

class AsyncLogging : noncopyable
{
public:
    AsyncLogging(const std::string basename, int flushInterval = 2);
    ~AsyncLogging()
    {
        if (running_)
            stop();
    }
    void append(const char *logline, int len);

    void start()
    {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop()
    {
        running_ = false;
        cond_.notify();
        thread_.join();
    }

private:
    void threadFunc();
    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::vector<std::shared_ptr<Buffer>> BufferVector;//muduo中使用的是unique_ptr
    typedef std::shared_ptr<Buffer> BufferPtr;
    const int flushInterval_;
    bool running_;
    std::string basename_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    BufferPtr currentBuffer_; //当前缓冲
    BufferPtr nextBuffer_;    //预备缓冲
    BufferVector buffers_;    //待写入文件的已填满的缓冲
    CountDownLatch latch_;
};
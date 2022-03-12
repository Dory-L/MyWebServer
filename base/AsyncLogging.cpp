#include "AsyncLogging.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include "LogFile.h"

AsyncLogging::AsyncLogging(std::string logFileName_, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(logFileName_),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),//bind绑定this，通过本对象调用
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),
      latch_(1)
{
    assert(logFileName_.size() > 1);
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

/*
*前端在生成一条日志消息的时候会调用append()
*/
void AsyncLogging::append(const char *logline, int len)
{
    MutexLockGuard lock(mutex_);
    if (currentBuffer_->avail() > len) //当前缓冲区足够
        currentBuffer_->append(logline, len);
    else
    { //当前缓冲区不够
        buffers_.push_back(currentBuffer_);
        currentBuffer_.reset();
        if (nextBuffer_)
            currentBuffer_ = std::move(nextBuffer_);
        else
            currentBuffer_.reset(new Buffer); //如果前端写入速度太快，一下就消耗完了两块缓冲，只好分配一块新的缓冲
        currentBuffer_->append(logline, len); //追加日志消息
        cond_.notify();                       //唤醒后端开始写入日志数据
    }
}

/*
*后端是新启动的线程，负责接收前端写入的数据
*/
void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_);
    BufferPtr newBuffer1(new Buffer); //准备好两块空闲的buffer，以备在临界区内交换
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            MutexLockGuard lock(mutex_);
            if (buffers_.empty()) // unusual usage! //等待条件触发
            {
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(currentBuffer_);
            currentBuffer_.reset();

            currentBuffer_ = std::move(newBuffer1); //立即将newbuffer1移为当前缓冲
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2); //newBuffer2替换nextbuffer_，这样前端始终有一个预备buffer可供调配，缩短前端临界区的长度。
            }
        }

        assert(!buffersToWrite.empty());

        if (buffersToWrite.size() > 25)
        {
            // char buf[256];
            // snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger
            // buffers\n",
            //          Timestamp::now().toFormattedString().c_str(),
            //          buffersToWrite.size()-2);
            // fputs(buf, stderr);
            // output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        for (size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }

        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }

        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}

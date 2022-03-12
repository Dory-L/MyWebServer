#include "Thread.h"
#include <assert.h>
#include <errno.h>
#include <linux/unistd.h>
#include <stdint.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#include "CurrentThread.h"

#include <iostream>
using namespace std;

namespace CurrentThread
{
    __thread int t_cachedTid = 0;
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char *t_threadName = "default";
}

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); } //linux/unistd.h

void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d", t_cachedTid);
    }
}

// 为了在线程中保留name,tid这些数据
struct ThreadData
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    string name_;
    pid_t *tid_;
    CountDownLatch *latch_;

    ThreadData(const ThreadFunc &func, const string &name, pid_t *tid,
               CountDownLatch *latch)
        : func_(func), name_(name), tid_(tid), latch_(latch) {}

    void runInThread()
    {
        *tid_ = CurrentThread::tid();//保存tid,使用完销毁指针
        tid_ = NULL;
        latch_->countDown();//计数器减一，用完销毁指针
        latch_ = NULL;

        CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);//设置线程名

        func_();//回调函数
        CurrentThread::t_threadName = "finished";
    }
};

void *startThread(void *obj)
{
    ThreadData *data = static_cast<ThreadData *>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

Thread::Thread(const ThreadFunc &func, const string &n)
    : started_(false),
      joined_(false),
      pthreadId_(0),
      tid_(0),
      func_(func),
      name_(n),
      latch_(1)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
        pthread_detach(pthreadId_);
}

void Thread::setDefaultName()
{
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread");
        name_ = buf;
    }
}

/***************************************************************
 * Funtion：start
 * Description：启动线程
***************************************************************/
void Thread::start()
{
    assert(!started_);
    started_ = true;
    ThreadData *data = new ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &startThread, data))
    {
        started_ = false;
        delete data;
    }
    else    //创建线程成功，等待计数器值为0
    {
        latch_.wait();
        assert(tid_ > 0);
    }
}

/***********************************
 * Function：join
 * Description: 等待线程执行结束
 * return: 成功返回0，失败返回对应的宏,EDEADLK 死锁,EINVAL,ESCRH 找不到指定的thread线程
***********************************/
int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}
/*
*CountDownLatch是一个计数器类，当计数器数值减为0时，所有受其影响的线程都会被激活
*/

#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int count)   //count只能在初始化时赋值，使用完之后不能被再次使用
    : mutex_(), condition_(mutex_), count_(count) {}

void CountDownLatch::wait()
{
    MutexLockGuard lock(mutex_);
    while (count_ > 0)
        condition_.wait();
}

void CountDownLatch::countDown()
{
    MutexLockGuard lock(mutex_);
    --count_;
    if (count_ == 0)
        condition_.notifyAll();
}
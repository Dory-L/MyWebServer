#include "LogStream.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <limits>

const char digits[] = "9876543210123456789";
const char *zero = digits + 9; //指向0

// From muduo
//将整数转换为字符串，并存放在buf处
template <typename T>
size_t convert(char buf[], T value)
{
    T i = value;
    char *p = buf;

    do
    {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd]; //zero为0123456789
    } while (i != 0);

    if (value < 0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

template <typename T>
void LogStream::formatInteger(T v)
{
    // buffer容不下kMaxNumericSize个字符的话会被直接丢弃
    if (buffer_.avail() >= kMaxNumericSize)
    {                                               //保证不会越界
        size_t len = convert(buffer_.current(), v); //
        buffer_.add(len);                           //移动_cur指针
    }
}

LogStream &LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream &LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(double v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {                                                                       //double和long double都保存32位
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v); //%g打印浮点数时，会去掉多余的零
        buffer_.add(len);
    }
    return *this;
}

LogStream &LogStream::operator<<(long double v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", v); //%L参数解释为双精度浮点数
        buffer_.add(len);
    }
    return *this;
}
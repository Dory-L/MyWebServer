#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

AppendFile::AppendFile(string filename) : fp_(fopen(filename.c_str(), "ae")) //“a”追加，“e” for O_CLOEXEC
{
    // 用户提供缓冲区
    setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile() { fclose(fp_); }//析构的时候关闭文件

//向文件写入长度为len的logline
void AppendFile::append(const char *logline, const size_t len)
{
    //写入文件
    size_t n = this->write(logline, len);
    size_t remain = len - n;
    while (remain > 0)
    {
        size_t x = this->write(logline + n, remain);
        if (x == 0)//如果写入失败，报错
        {
            int err = ferror(fp_);
            if (err)
                fprintf(stderr, "AppendFile::append() failed !\n");
            break;
        }
        n += x;
        remain = len - n;
    }
}

//刷新文件流
void AppendFile::flush() { fflush(fp_); }

size_t AppendFile::write(const char *logline, size_t len)
{
    return fwrite_unlocked(logline, 1, len, fp_);//fwrite的线程不安全版本
}
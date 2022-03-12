//Logging是对外接口，Logging类内涵一个LogStream对象，主要是为了每次打log的时候在log之前和之后加上固定的格式化的信息，比如打log的行、文件名等信息。
#pragma once
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "LogStream.h"

class AsyncLogging;

class Logger
{
public:
    Logger(const char *fileName, int line);
    ~Logger();
    LogStream &stream() { return impl_.stream_; }

    static void setLogFileName(std::string fileName) { logFileName_ = fileName; }
    static std::string getLogFileName() { return logFileName_; }

private:
    class Impl
    {
    public:
        Impl(const char *fileName, int line);
        void formatTime();

        LogStream stream_;
        int line_;
        std::string basename_;
    };
    Impl impl_;
    static std::string logFileName_;
};

#define LOG Logger(__FILE__, __LINE__).stream()
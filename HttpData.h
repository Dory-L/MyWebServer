//连接类，在其中读取数据，处理数据，分发数据
#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include "Timer.h"

class EventLoop;
class TimerNode;
class Channel;

//处理状态
enum ProcessState
{
    STATE_PARSE_URI = 1,
    STATE_PARSE_HEADERS,
    STATE_RECV_BODY,
    STATE_ANALYSIS,
    STATE_FINISH
};

//URI状态
enum URIState
{
    PARSE_URI_AGAIN = 1,
    PARSE_URI_ERROR,
    PARSE_URI_SUCCESS,
};

//头状态
enum HeaderState
{
    PARSE_HEADER_SUCCESS = 1,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

enum AnalysisState
{
    ANALYSIS_SUCCESS = 1,
    ANALYSIS_ERROR
};

enum ParseState
{
    H_START = 0,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF
};

//连接状态
enum ConnectionState
{
    H_CONNECTED = 0,
    H_DISCONNECTING,
    H_DISCONNECTED
};

//http方法
enum HttpMethod
{
    METHOD_POST = 1,
    METHOD_GET,
    METHOD_HEAD
};

//http版本
enum HttpVersion
{
    HTTP_10 = 1,
    HTTP_11
};

//MIME类型
class MimeType
{
private:
    static void init();
    static std::unordered_map<std::string, std::string> mime;
    MimeType();
    MimeType(const MimeType &m);

public:
    static std::string getMime(const std::string &suffix);

private:
    static pthread_once_t once_control;
};

class HttpData : public std::enable_shared_from_this<HttpData>
{
public:
    HttpData(EventLoop *loop, int connfd);
    ~HttpData() { close(fd_); }
    void reset();
    void seperateTimer();
    void linkTimer(std::shared_ptr<TimerNode> mtimer)
    {
        // shared_ptr重载了bool, 但weak_ptr没有
        timer_ = mtimer;
    }
    std::shared_ptr<Channel> getChannel() { return channel_; }
    EventLoop *getLoop() { return loop_; }
    void handleClose();
    void newEvent();

private:
    EventLoop *loop_;
    std::shared_ptr<Channel> channel_;
    int fd_;
    std::string inBuffer_;
    std::string outBuffer_;
    bool error_;
    ConnectionState connectionState_;

    //http method
    HttpMethod method_;
    //http version
    HttpVersion HTTPVersion_;
    std::string fileName_;
    std::string path_;
    //当前读取位置
    int nowReadPos_;
    ProcessState state_;
    ParseState hState_;
    //保活标志
    bool keepAlive_;
    std::map<std::string, std::string> headers_;
    std::weak_ptr<TimerNode> timer_;

    void handleRead();
    void handleWrite();
    void handleConn();
    void handleError(int fd, int err_num, std::string short_msg);
    URIState parseURI();
    HeaderState parseHeaders();
    AnalysisState analysisRequest();
};
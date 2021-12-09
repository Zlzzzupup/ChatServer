#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

class ChatServer
{
private:
    muduo::net::TcpServer mServer; // 组合muduo库，实现服务器功能的类对象
    muduo::net::EventLoop *mLoop;  // 指向事件循环对象的指针

    // 上报链接相关信息的回调函数
    void onConnection(const muduo::net::TcpConnectionPtr &);
    // 上报读写事件相关信息的回调函数
    void onMessage(const muduo::net::TcpConnectionPtr &,
                   muduo::net::Buffer *,
                   muduo::Timestamp);

public:
    // 初始化聊天服务器对象
    ChatServer(muduo::net::EventLoop *loop,
               const muduo::net::InetAddress &listenAddr,
               const std::string &nameArg);

    // 启动服务
    void start();
};

#endif
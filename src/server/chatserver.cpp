#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"

#include <functional>
#include <string>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(muduo::net::EventLoop *loop,
                       const muduo::net::InetAddress &listenAddr,
                       const std::string &nameArg)
    : mServer(loop, listenAddr, nameArg),
      mLoop(loop)
{
    // 注册连接回调
    mServer.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    mServer.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    mServer.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    mServer.start();
}

// 上报连接相关信息的回调函数
void ChatServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    // 客户端断开连接
    if (!conn->connected())
    {
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buffer,
                           muduo::Timestamp time)
{
    string buf = buffer->retrieveAllAsString();

    // 数据的反序列化，解码
    json js = json::parse(buf);

    // 目的: 完成解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取 -> 业务handler -> conn js time
    MsgHandler msgHandler = ChatService::getInstance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理起，来执行相应的业务处理器
    msgHandler(conn, js, time);
}

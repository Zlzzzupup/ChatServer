/**
 * @file muduoserver.cpp
 * @author zlz
 * @brief muduoserver配置
 * @version 0.1
 * @date 2021-12-07
 *
 * @copyright Copyright (c) 2021
 *
 */

/**
 * @brief muduo网络库给用户提供了两个主要的类
 * TcpServer: 用于编写服务器程序
 * TcpClient: 用于编写客户端程序
 *
 * epoll + 线程池
 * 好处：能够把网络I/O的代码和业务代码区分开
 */

#include <iostream>
#include <functional>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace std;
using namespace placeholders;

/**
 * @brief 基于muduo网络库开发服务器程序
 * 1. 组合TcpServer对象
 * 2. 创建EventLoop事件循环对象的指针
 * 3. 明确TcpServer的构造函数需要什么参数，输出ChatServer的构造函数
 * 4. 在当前服务器类的构造函数中，注册处理连接的回调函数的处理读写事件函数
 * 5. 设置合适的服务器线程数，muduo库会自己分配I/O线程和worker线程
 */
class ChatServer
{
public:
    ChatServer(muduo::net::EventLoop *loop,               // 事件循环
               const muduo::net::InetAddress &listenAddr, // IP + Port
               const string &nameArg)                     // 服务器名
        : mServer(loop, listenAddr, nameArg),
          mLoop(loop)
    {
        // 给服务器注册用户连接的创建和断开回调
        mServer.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        // 给服务器注册用户读写事件回调
        mServer.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务器的线程数量 1个IO线程 3个worker线程
        mServer.setThreadNum(4);
    }

    // 开启事件循环
    void start()
    {
        mServer.start();
    }

private:
    // 专业处理用户的连接创建和断开
    void onConnection(const muduo::net::TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << " -> "
                 << conn->localAddress().toIpPort() << " STATE:ONLINE" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << " -> "
                 << conn->localAddress().toIpPort() << " STATE:OFFLINE" << endl;

            conn->shutdown(); // 相当于socket中的close(fd)
        }
    }

    // 专门处理用户的读写时间
    void onMessage(const muduo::net::TcpConnectionPtr &conn, // 连接
                   muduo::net::Buffer *buf,                  // 缓冲区
                   muduo::Timestamp time)                    // 接收到数据的时间信息
    {
        string buffer = buf->retrieveAllAsString();
        cout << "recv data: " << buffer
             << "time: " << time.toString() << endl;

        // 再原样发送回去
        conn->send(buf);
    }

    muduo::net::TcpServer mServer;
    muduo::net::EventLoop *mLoop;
};

int main()
{
    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr("127.0.0.1", 1234);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop(); // epoll_wait以阻塞方式等待新用户连接、已连接用户的读写事件等

    return 0;
}

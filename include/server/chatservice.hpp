#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr &conn,
                                      json &js,
                                      muduo::Timestamp)>;

// 聊天服务器业务类(设置为单例模式)
class ChatService
{
public:
    // 获取单例模式的实例
    static ChatService *getInstance();

    // 处理登录业务代码
    void login(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 处理注册业务
    void reg(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

private:
    ChatService();

    // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> mMsgHandlerMap;

    // 存储在线用户的通信连接
    unordered_map<int, muduo::net::TcpConnectionPtr> mUserConnMap;

    // 定义互斥锁，保证mUserConnMap的线程安全
    mutex mConnMutex;

    // 数据操作类对象
    UserModel mUserModel;
};

#endif
#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemsgmodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

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
    // 处理一对一聊天业务
    void oneChat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 处理好友添加业务
    void addFriend(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 处理创建群组业务
    void createGroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 处理加入群组业务
    void addGroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 处理群组聊天业务
    void groupChat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 处理用户下线业务
    void loginout(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 处理客户端异常退出
    void clientCloseException(const muduo::net::TcpConnectionPtr &conn);
    // 处理服务器异常退出
    void reset();

    // redis订阅回调函数
    void handlerRedisSubscribeMSG(int, string);

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

    // 数据库操作类对象
    UserModel mUserModel;
    OfflineMsgModel mOfflineMsgModel;
    FriendModel mFriendModel;
    GroupModel mGroupModel;

    // redis操作对象
    Redis mRedis;    
};

#endif
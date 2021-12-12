#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>

// 获取单例对象的借口函数
ChatService *ChatService::getInstance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的handler回调操作
ChatService::ChatService()
{
    mMsgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    mMsgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = mMsgHandlerMap.find(msgid);
    if (it == mMsgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp) {
            LOG_ERROR << "msgid: " << msgid << " can not find handler!";
        };
    }
    else
    {
        return mMsgHandlerMap[msgid];
    }
}

// 处理登录业务 id  password
void ChatService::login(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int id = js["id"].get<int>();
    string password = js["password"];

    User user = mUserModel.query(id);
    if (user.getId() == id && user.getPassword() == password)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json respone;
            respone["msgid"] = LOGIN_MSG_ACK;
            respone["errno"] = 2;
            respone["errmsg"] = "该账号已经登录，请重新输入新账号";

            conn->send(respone.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(mConnMutex);
                mUserConnMap.insert({id, conn});
            }

            // 登录成功，更新用户状态state offline -> online
            user.setState("online");
            mUserModel.updateState(user);

            json respone;
            respone["msgid"] = LOGIN_MSG_ACK;
            respone["errno"] = 0;
            respone["id"] = user.getId();
            respone["name"] = user.getName();

            conn->send(respone.dump());
        }
    }
    else if (user.getId() == id && user.getPassword() != password)
    {
        // 密码错误，登录失败
        json respone;
        respone["msgid"] = LOGIN_MSG_ACK;
        respone["errno"] = 2;
        respone["errmsg"] = "密码错误，请重新登录";

        conn->send(respone.dump());
    }
    else
    {
        // 该用户不存在，登录失败
        json respone;
        respone["msgid"] = LOGIN_MSG_ACK;
        respone["errno"] = 2;
        respone["errmsg"] = "用户不存在，请重新登录";

        conn->send(respone.dump());
    }
}

// 处理注册业务 name password
void ChatService::reg(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{

    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = mUserModel.insert(user);

    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 0;
        response["errmsg"] = "注册成功";

        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "注册失败";

        conn->send(response.dump());
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const muduo::net::TcpConnectionPtr &conn)
{
    User user;

    {
        lock_guard<mutex> lock(mConnMutex);
        for (auto it = mUserConnMap.begin(); it != mUserConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                // 从map表中删除用户的连接信息
                user.setId(it->first);
                mUserConnMap.erase(it);
                break;
            }
        }
    }

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        mUserModel.updateState(user);
    }
}
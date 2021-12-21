#include "chatservice.hpp"
#include "public.hpp"
#include <vector>
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
    // 用户业务相关事件处理函数回调
    mMsgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    mMsgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    mMsgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    mMsgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    mMsgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    mMsgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    mMsgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    mMsgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    // 连接redis服务器
    if (mRedis.connect())
    {
        // 设置上报消息回调函数
        mRedis.init_notify_handler(std::bind(&ChatService::handlerRedisSubscribeMSG, this, _1, _2));
    }
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

            // 登录成功后向redis订阅channel（也就是id）
            mRedis.subscribe(id);

            json respone;
            respone["msgid"] = LOGIN_MSG_ACK;
            respone["errno"] = 0;
            respone["id"] = user.getId();
            respone["name"] = user.getName();

            // 查询该用户是否有离线消息 2021-12-12
            vector<string> vec = mOfflineMsgModel.query(id);
            if (!vec.empty())
            {
                respone["offlinemsg"] = vec;

                //  读取该用户的离线消息后，把该用户的离校消息清空
                mOfflineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回 2021-12-13
            vector<User> userVec = mFriendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (auto &user : userVec)
                {
                    json jsuser;
                    jsuser["id"] = user.getId();
                    jsuser["name"] = user.getName();
                    jsuser["state"] = user.getState();
                    vec2.push_back(jsuser.dump());
                }

                respone["friends"] = vec2;
            }

            // 查询用户的群组信息 2021-12-14
            vector<Group> groupuserVec = mGroupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();

                    // 获取每个组里有哪些user
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();

                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                respone["groups"] = groupV;
            }

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

    // 用户注销，相当于就是下线，在redis中取消订阅通道 2021-12-21
    mRedis.unsubscribe(user.getId());

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        mUserModel.updateState(user);
    }
}

// 处理一对一聊天业务 msgid id from to msg
void ChatService::oneChat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(mConnMutex);
        auto it = mUserConnMap.find(toid);

        if (it != mUserConnMap.end())
        {
            // 对方在线，转发消息，服务器自动转发给对方
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线，表示在线但是不在本台主机中用redis转发 2021-12-21
    User user = mUserModel.query(toid);
    if ("online" == user.getState())
    {
        mRedis.publish(toid, js.dump());
        return;
    }

    // 对方不在线，转发离线消息
    mOfflineMsgModel.insert(toid, js.dump());
}

// 处理服务器异常退出,业务重置方法
void ChatService::reset()
{
    // 把online状态的用户，设置为offline
    mUserModel.resetState();
}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    mFriendModel.insert(userid, friendid);
}

// 创建群组业务 msgid id groupname groupdesc
void ChatService::createGroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, groupname, groupdesc);
    if (mGroupModel.createGroup(group))
    {
        // 存储群组创建人信息
        mGroupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    mGroupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务 msgid id groupid msg
void ChatService::groupChat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> userIdVec = mGroupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(mConnMutex);
    for (int id : userIdVec)
    {
        auto it = mUserConnMap.find(id);

        if (it != mUserConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线，表示在线但是不在本台主机中用redis转发 2021-12-21
            User user = mUserModel.query(id);
            if ("online" == user.getState())
            {
                mRedis.publish(id, js.dump());
            }
            else
            {
                // 存储离线消息
                mOfflineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 用户下线业务
void ChatService::loginout(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(mConnMutex);
        auto it = mUserConnMap.find(userid);
        if (it != mUserConnMap.end())
        {
            mUserConnMap.erase(it);
        }
    }

    // 用户注销，相当于下线，在redis中取消订阅通道  2021-12-21
    mRedis.unsubscribe(userid);

    // 更新用户信息
    User user(userid, "", "", "offline");
    mUserModel.updateState(user);
}

// 从redis消息队列中获取订阅的消息 2021-12-21
// 消息就被转发连接所在的这台主机上来了
void ChatService::handlerRedisSubscribeMSG(int userid, string msg)
{
    lock_guard<mutex> lock(mConnMutex);
    auto it = mUserConnMap.find(userid);
    if (it != mUserConnMap.end())
    {
        it->second->send(msg);
        return ;
    }

    mOfflineMsgModel.insert(userid, msg);
}
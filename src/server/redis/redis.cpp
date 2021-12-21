#include "redis.hpp"
#include <iostream>

using namespace std;

Redis::Redis()
    :mPublish_Context(nullptr), mSubcribe_Context(nullptr)
{
}

Redis::~Redis()
{
    if (mPublish_Context != nullptr)
    {
        redisFree(mPublish_Context);
    }

    if (mSubcribe_Context != nullptr)
    {
        redisFree(mSubcribe_Context);
    }
}

bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    mPublish_Context = redisConnect("127.0.0.1", 6379);
    if (nullptr == mPublish_Context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责 subscribe订阅消息的上下文连接
    mSubcribe_Context = redisConnect("127.0.0.1", 6379);
    if (nullptr == mSubcribe_Context)
    {
        cerr << "subcribe redis failed!" << endl;
        return false;
    }

    // 在单独线程中，监听通道上的事件，有消息给业务层上报
    thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;
    return true;
}

// 向redis指定通道channel发送消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply*)redisCommand(mPublish_Context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }

    freeReplyObject(reply);
    return true;
}

// 向redis执行通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    // subscribe命令本身会造成线程阻塞等到通道里面发生消息，这里只做订阅，不接受消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis-server相应消息，否则和notifyMsg线程抢占相应资源
    if (REDIS_ERR == redisAppendCommand(this->mSubcribe_Context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }

    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕(done被置为1)
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->mSubcribe_Context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }

    // redisGetReply
    return true;
}

// 向redis执行通道subscribe订阅消息
bool Redis::unsubscribe(int channel)
{
    // subscribe命令本身会造成线程阻塞等到通道里面发生消息，这里只做订阅，不接受消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis-server相应消息，否则和notifyMsg线程抢占相应资源
    if (REDIS_ERR == redisAppendCommand(this->mSubcribe_Context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }

    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕(done被置为1)
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->mSubcribe_Context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }

    // redisGetReply
    return true;
}

// 独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->mSubcribe_Context, (void**)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            mNotify_Message_Handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}

void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->mNotify_Message_Handler = fn;
}
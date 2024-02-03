#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>

// 获取单例
// 类外定义成员函数时，不能指定static，否则编译器当做新的与类无关的静态函数, 编译出错
ChatService& ChatService::get_instance()
{
    static ChatService service;
    return service;
}

// 注册消息及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回空的默认处理器
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp ts)
        {
            LOG_ERROR << "msgid: " << msgid << " can not find handler!";
        };
    }

    return _msgHandlerMap[msgid];
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do login service !";
}
// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do reg service !";
}

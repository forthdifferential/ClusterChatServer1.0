#include "chatservice.hpp"
#include "user.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>

// 获取单例
// 类外定义成员函数时，不能指定static，否则编译器当做新的与类无关的静态函数, 编译出错
ChatService &ChatService::get_instance()
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

// 把业务层和数据层分离 采用DAO设计模式，对数据的访问和操作封装在DAO类中，业务逻辑层关注数据处理而不关注数据存储的细节
// 数据层 用的ORM，对象关系映射，面向对象的方式来操作数据库
// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do login service !";
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    json response;
    response["msgid"] = REG_MSG_ACK;

    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该账号已经登录，请重新输入新账号
            response["errno"] = 1;
            response["errmsg"] = "该账号已经登录，请重新输入新账号";
        }
        else
        {
            // 登录成功
            // 状态更新
            user.setState("online");
            _userModel.updateState(user);

            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
        }
    }
    else if (user.getId() == id && user.getPwd() != pwd)
    {
        // 密码错误
        response["errno"] = 1;
        response["errmsg"] = "密码错误";
    }
    else
    {
        // 用户不存在
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名不存在";
    }
    conn->send(response.dump());
}
// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do reg service !";
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

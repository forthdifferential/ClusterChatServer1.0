#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "../../thirdparty/json.hpp"
using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp ts)>;

// 聊天服务器业务类
class ChatService
{
private:
    ChatService(/* args */);
    
public:
    // 获取单例
    static ChatService& get_instance(); 
    ~ChatService();
    // 处理登录业务
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

private:
    // 存储消息id和其对应的业务处理方法 
    unordered_map<int, MsgHandler> _msgHandlerMap;
};



#endif
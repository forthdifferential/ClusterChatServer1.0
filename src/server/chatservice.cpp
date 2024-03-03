#include "chatservice.hpp"
#include "user.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>

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
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect()){
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
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
    response["msgid"] = LOGIN_MSG_ACK;

    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该账号已经登录，不允许重复登录
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            // LOG_INFO << __FILE__ << ":" << __LINE__ << ":"<< "this account is using, input another!";
        }
        else
        { // 登录成功
            // 记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // 订阅channel(id)
            _redis.subscribe(id);

            // 用户状态更新
            user.setState("online");
            _userModel.updateState(user); // mysql Server保证线程安全

            // BaseInfo消息响应
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 上线后处理离线消息
            vector<string> offlineMsgVec = _offlineMsgModel.query(id);
            if (!offlineMsgVec.empty())
            {
                response["offlinemsg"] = offlineMsgVec;
                _offlineMsgModel.remove(id);
            }
            // 查询该用户的好友信息
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> friendsTempVec;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    friendsTempVec.push_back(js.dump());
                }
                response["friends"] = friendsTempVec;
            }
            // 查询该用户的群组信息
            vector<Group> groupsVec = _groupModel.queryGroups(id);
            if (!groupsVec.empty())
            {
                vector<string> groupTempVec;
                for (Group &group : groupsVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userTempVec;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userTempVec.push_back(js.dump());
                    }
                    grpjson["users"] = userTempVec;
                    groupTempVec.push_back(grpjson.dump());
                }

                response["groups"] = groupTempVec;
            }
        }
    }
    else if (user.getId() == id && user.getPwd() != pwd)
    {
        // 用户存在，但是密码错误
        response["errno"] = 3;
        response["errmsg"] = "password wrong!";
        // LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << "密码错误!";
    }
    else
    {
        // 用户不存在
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        // LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << "用户名不存在!"; 
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
    json response;
    response["msgid"] = REG_MSG_ACK;
    if (state)
    { // 注册成功 
        response["errno"] = 0;
        response["id"] = user.getId();
    }
    else
    {// 注册失败     
        response["errno"] = 1;
    }
    conn->send(response.dump());
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 取消redis订阅
    _redis.unsubscribe(userid);

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息 服务器推送
            it->second->send(js.dump());
            return;
        }
    }
    // 查询todid是否在线，跨服务器通信
    User user = _userModel.query(toid);
    if(user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}
// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        { // 转发群消息 
            it->second->send(js.dump());
        }
        else
        {  
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {   // 查询toid是否在线
                _redis.publish(id, js.dump());
            }
            else 
            {// 存储离线群消息   
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);

        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从连接表删除用户
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 取消redis订阅
    _redis.unsubscribe(user.getId());

    // 更新表内用户状态
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
             << "服务器主动关闭，正常退出";

    // 异常退出，重置offline状态
    _userModel.resetState();
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}
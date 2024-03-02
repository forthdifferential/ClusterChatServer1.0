#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
using namespace std;


void resetHandler(int){
    ChatService::get_instance().reset();
    exit(0);
}

int main()
{
    signal(SIGINT, resetHandler);

    // 调用muduo库创建基于事件驱动 IO多路复用 + 线程池的网络
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;

}
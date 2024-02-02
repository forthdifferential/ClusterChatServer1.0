#include "json.hpp"

using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
using namespace std;

// json 序列化示例1
// 内存存键值对，是无序的
string fun1() {
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?";

    return js.dump();
}

// json 序列化示例2
// 多维json
string fun2() {
    json js;
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["li si"] = "hello json";

    return js.dump();
}

// json 序列化示例2
// 容器序列化
string fun3(){
    json js;
    vector<int> vec{1,2,3,4};
    map<int, string> m{{1, "黄山"},{2,"华山"}, {3, "泰山"}};
    js["path"] = m;
    js["list"] = vec;
    json jsmsg;
    jsmsg["a"] = "1";
    js["msg"] = jsmsg;

    return js.dump();
}


int main() {

    // string recvBuf = fun1();
    // json jsbuf = json::parse(recvBuf);
    // cout << jsbuf["msg_type"] << endl;

    // string recvBuf = fun2();
    // json jsbuf = json::parse(recvBuf);
    // cout << jsbuf["msg"] << endl;

    string recvBuf = fun3();
    json jsbuf = json::parse(recvBuf);

    cout << jsbuf["list"] << endl;

    return 0;
}
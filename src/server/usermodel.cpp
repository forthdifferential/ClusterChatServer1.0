#include "usermodel.hpp"
#include "db.h"
#include <iostream>

using namespace std;



// User表的增加方法
bool UserModel::insert(User& user){
    // 1. 组装SQL
    char sql[1024] = {0};
    sprintf(sql,"insert into User(name, password, state) values('%s', '%s', '%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    
    // 2. 定义Mysql对象
    MySQL mysql;
    if (mysql.connect()){
        if (mysql.update(sql)) {
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
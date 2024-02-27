#include "usermodel.hpp"
#include "db.h"
#include <iostream>

using namespace std;

// User表的增加方法
bool UserModel::insert(User &user)
{
    // 1. 组装SQL
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    // 2. 定义Mysql对象
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 根据用户号码查询用户信息
User UserModel::query(int id)
{
    // 1. 组装SQL
    char sql[1024] = {0};
    // TODO mysql注入攻击
    sprintf(sql, "select * from User where id = %d", id);
    // 2. 定义Mysql对象
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                // TODO 不太懂 mysql释放结果集
                mysql_free_result(res);
                return user;
            }
        }
    }

    return User();
}

// 更新用户的状态信息
bool UserModel::updateState(User &user)
{
    // 1. 组装SQL
    char sql[1024] = {0};
    // TODO 不太懂 构建 SQL 查询语句时，将字符串类型的字段值用单引号括起来是 SQL 语法的要求
    sprintf(sql, "update User set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    // 2. 定义Mysql对象
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}
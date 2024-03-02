#ifndef GROUP_H
#define GROOUP_H

#include <string>
#include <vector>
#include "groupuser.hpp"
using namespace std;

class Group
{
public:
    Group(int id = -1, string name = "", string desc = "") : _id(id), _name(name), _desc(desc) {}
    void setId(const int id) { _id = id; }
    void setName(const string name) { _name = name; }
    void setDesc(const string desc) { _desc = desc; }

    int getId() const { return _id; }
    string getName() const { return this->_name; }
    string getDesc() const { return this->_desc; }
    vector<GroupUser> &getUsers() { return _users; }

private:
    int _id;
    string _name;
    string _desc;
    vector<GroupUser> _users;
};

#endif
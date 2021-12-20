#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// 群组成员，多一个role角色信息，从user中直接继承，复用user中的其他信息
class GroupUser : public User
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() { return role; }

private:
    string role;
};

#endif
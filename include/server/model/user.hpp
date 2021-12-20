#ifndef USER_H
#define USER_H

#include <string>

using namespace std;

class User
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    {
        this->id = id;
        this->name = name;
        this->password = pwd;
        this->state = state;
    }

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPassword(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPassword() { return this->password; }
    string getState() { return this->state; }

// 2021-12-13 加入了groupuser子类，
// 为了能将信息继承，将属性从private改为protected
protected:
    int id;
    string name;
    string password;
    string state;
};

#endif
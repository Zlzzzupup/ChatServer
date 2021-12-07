#include "json.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <map>

using json = nlohmann::json;
using namespace std;

string func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?";

    string sendBuf = js.dump(); // json数据对象-> json字符串
    cout << sendBuf.c_str() << endl;
    cout << js << endl;

    return sendBuf;
}

// json序列化示例2
void func2()
{
    json js;
    // 添加数组
    js["id"] = {1, 2, 3, 4};
    // 添加key- value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san"}, {"hello world"}, {"liu shuo"}, {"hello china"}};

    cout << js << endl;
}

// json序列化示例3
string func3()
{
    json js;

    // 直接序列化一个vector容器
    vector<int> nums;
    nums.push_back(1);
    nums.push_back(1);
    nums.push_back(1);
    nums.push_back(1);

    js["list"] = nums;

    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "黄山"});
    m.insert({3, "黄山"});
    m.insert({4, "黄山"});

    js["path"] = m;

    cout << js << endl;
    string sendBuf = js.dump();
    return sendBuf;
}

int main()
{
    string recvBuf = func1();
    json jBuf = json::parse(recvBuf);
    cout << jBuf["msg_type"] << endl;
    cout << jBuf["from"] << endl;
    cout << jBuf["to"] << endl;
    cout << jBuf["msg"] << endl;

    string recvBuf3 = func3();
    jBuf = json::parse(recvBuf3);
    vector<int> nums = jBuf["list"];
    
    for (int num : nums)
        cout << num << " ";
    cout << endl;
    
    return 0;
}
// 小彭老师作业05：假装是多线程 HTTP 服务器 - 富连网大厂面试官觉得很赞
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <chrono>
#include <shared_mutex>
#include <mutex>
#include <future>


struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::map<std::string, User> users;
// std::map<std::string, long> has_login;  // 换成 std::chrono::seconds 之类的

std::map<std::string, std::chrono::steady_clock::time_point> has_login;
std::shared_mutex user_mutex;
std::shared_mutex login_mutex;



// 作业要求1：把这些函数变成多线程安全的
// 提示：能正确利用 shared_mutex 加分，用 lock_guard 系列加分
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    std::unique_lock lock(user_mutex);
    User user = {password, school, phone};
    if (users.emplace(username, user).second)
        return "注册成功";
    else
        return "用户名已被注册";
}

using double_ms = std::chrono::duration<double, std::milli>;
std::string do_login(std::string username, std::string password) {
    // 作业要求2：把这个登录计时器改成基于 chrono 的
    
   
    {   
        std::unique_lock lock(login_mutex); // ensure has_login[username] = t1 without data race
        auto t1 = std::chrono::steady_clock::now();
        if (has_login.find(username) != has_login.end()) {
            auto diff = t1 - has_login[username];
            double msec = std::chrono::duration_cast<double_ms>(diff).count();
            return std::to_string(msec) + "ms内登录过";
        }
        has_login[username] = t1;
    }
    // has_login[username] = now;
    // {  
    //     std::unique_lock lock(login_mutex);
        
    // }
    
    std::shared_lock lock(user_mutex); // shared_lock确保.find的时候没有write
    if (users.find(username) == users.end())
        return "用户名错误";
    if (users.at(username).password != password)
        return "密码错误";
    return "登录成功";
}

std::string do_queryuser(std::string username) {
    std::shared_lock lock(user_mutex);
     if (users.find(username) == users.end()) return "user name does not exist";
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "用户名: " << username << std::endl;
    ss << "学校:" << user.school << std::endl;
    ss << "电话: " << user.phone << std::endl;
    return ss.str();
}


struct ThreadPool {
    std::vector<std::future<void>> futures;
    void create(std::function<void()> start) {
        // 作业要求3：如何让这个线程保持在后台执行不要退出？
        // 提示：改成 async 和 future 且用法正确也可以加分
        // not sure if to std::launch::aynsc here, too many threads might get it stuck
        // std::async(function<void()>) will return a future element
        futures.emplace_back(std::async(start));
    }
    void waitForAll() {
        for (auto &f : futures) {
            f.get(); // wait for all to happen
        }
    }
};

// 下面这样写应该也可以，
// struct ThreadPool {
//     // 动态管理vector大小，自动destruct
//     std::unique_ptr<std::vector<std::thread>> threads;

//     ThreadPool() : threads(std::make_unique<std::vector<std::thread>>()) {}

//     void create(std::function<void()> start) {
//         threads->emplace_back(std::thread(start));
//     }

//     void waitForAll() {
//         for (auto &t : *threads) {
//             if (t.joinable()) {
//                 t.join();  // 等待所有线程完成
//             }
//         }
//     }
// };

ThreadPool tpool;


namespace test {  // 测试用例？出水用力！
std::string username[] = {"张心欣", "王鑫磊", "彭于斌", "胡原名"};
std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
std::string school[] = {"九百八十五大鞋", "浙江大鞋", "剑桥大鞋", "麻绳理工鞋院"};
std::string phone[] = {"110", "119", "120", "12315"};
}

int main() {
    for (int i = 0; i < 262144; i++) {
        // call register
        tpool.create([&] {
            std::cout << do_register(test::username[rand() % 4], test::password[rand() % 4], test::school[rand() % 4], test::phone[rand() % 4]) << std::endl;
        });
        
        // call login
        tpool.create([&] {
            std::cout << do_login(test::username[rand() % 4], test::password[rand() % 4]) << std::endl;
        });
        
        // user might be queried to wait
        tpool.create([&] {
            std::cout << do_queryuser(test::username[rand() % 4]) << std::endl;
        });
    }

    // 作业要求4：等待 tpool 中所有线程都结束后再退出
    tpool.waitForAll();
    return 0;
}
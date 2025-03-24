#pragma once
#include "Util.hpp"
#include<memory>
#include<mutex>
// 该类用于读取配置文件的信息
namespace storge{
    // 懒汉模式
    const char *config_file = "Storage.conf";
    class Config{
    private:
        int server_port_;
        std::string server_ip
    };
}
#pragma once
#include<jsoncpp/json/json.h>
#include"Util.hpp"
namespace mylog{
    const std::string Config_File = "../../logSystem/logsCode/config.conf";
    class Config{
    private:
    Config(){
            std::string content;
            if(mylog::Util::File::GetContent(content,Config_File)==false){
                std::cerr << __FILE__ << __LINE__  << std::endl;
                perror("open config.conf failed:");
            }
            Json::Value root;
            mylog::Util::JsonUtil::UnSerialize(content, &root);
            buffer_size = root["buffer_size"].asInt64();
            threshold = root["threshold"].asInt64();
            linear_growth = root["linear_growth"].asInt64();
            flush_log = root["flush_log"].asInt64();
            backup_addr = root["backup_addr"].asString();
            backup_port = root["backup_port"].asInt();
            thread_count = root["thread_count"].asInt();
        }
    public:
        // 单例模式
        static Config* GetConfig(){
            // C++11保证了局部静态变量的初始化是线程安全的。
            // 避免指针构造
            static Config jsonData;
            return &jsonData;
        }
        size_t buffer_size;//缓冲区基础容量
        size_t threshold;// 倍数扩容阈值
        size_t linear_growth;// 线性增长容量
        size_t flush_log;//控制日志同步到磁盘的时机，默认为0,1调用fflush，2调用fsync
        std::string backup_addr;
        uint16_t backup_port;
        size_t thread_count;
    };//JsonData
}
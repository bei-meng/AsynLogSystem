#pragma once
#include<string>
class JsonConfigPath {
public:
    static void SetConfigPath(const std::string& path) { jsonconfigPath_ = path; }
    static const std::string& GetConfigPath() { return jsonconfigPath_; }

private:
    static std::string jsonconfigPath_;
};
    
// 静态成员初始化
std::string JsonConfigPath::jsonconfigPath_ = "../../log_system/logs_code/config.conf";
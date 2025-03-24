#pragma once
#include<memory>
#include<thread>

#include "Level.hpp"
#include "Util.hpp"

namespace mylog{
class LogMessage{
private:
    size_t line_;               // 行号
    time_t ctime_;              // 时间
    std::string filename_;      // 文件名
    std::string name_;          // 日志器名
    std::string payload_;       // 信息体
    std::thread::id tid_;       // 线程id
    LogLevel::value level_;     // 等级
public:
    using ptr = std::shared_ptr<LogMessage>;
    LogMessage()=default;
    LogMessage(const LogLevel::value level,const std::string &file,
    const size_t line,const std::string &name,const std::string &payload)
    :   name_(name),
        filename_(file),
        payload_(payload),
        level_(level),
        line_(line),
        ctime_(Util::Date::Now()),
        tid_(std::this_thread::get_id()){}
    
    std::string format(){
        std::stringstream ss;
        // 获取当前时间
        struct tm t;
        localtime_r(&ctime_,&t);
        char buf[128];
        strftime(buf,sizeof(buf),"%H:%M:%S",&t);
        std::string tmp1 = '[' + std::string(buf) + "][";
        std::string tmp2 = '[' + std::string(LogLevel::ToString(level_))+"][" +name_+"]["+ filename_ 
            +":" + std::to_string(line_) + "]\t" + payload_ +"\n";
        ss<<tmp1<<tid_<<tmp2;
        return ss.str();
    }
};
}
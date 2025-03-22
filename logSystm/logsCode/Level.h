#pragma once
#include<string>

namespace mylog{
class LogLevel{
public:
    // 这里使用强枚举类型，类型更安全
    enum class value{DEBUG,INFO,WARN,ERROR,FATAL};

    // 日志等级的字符串转换接口
    static const char* ToSring(value level){
        switch(level){
            case value::DEBUG:
                return "DEBUG";
            case value::INFO:
                return "INFOR";
            case value::WARN:
                return "WARN";
            case value::ERROR:
                return "ERROR";
            case value::FATAL:
                return "FATAL";
            default:
                return "UNKNOW";
        }
        return "UNKNOW";
    }
};
}
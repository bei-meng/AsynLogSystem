#pragma once
#include<string>

namespace mylog{
class LogLevel{
public:
    // 这里使用强枚举类型，类型更安全
    enum class value{DEBUG,INFO,WARN,ERROR,FATAL};

    // 日志等级的字符串转换接口
};
}
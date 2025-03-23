#include<unordered_map>
#include"AsyncLogger.hpp"

namespace mylog{
// 通过单例对象对日志器进行管理,静态局部变量实现
class LoggerManager{
private:
    std::mutex mtx_;
    // 默认日志器
    AsyncLogger::ptr default_logger_;
    // 存放日志器
    std::unordered_map<std::string, AsyncLogger::ptr> loggers_;

    LoggerManager(){
        std::unique_ptr<LoggerBuilder> builder(new LoggerBuilder());
        builder->BuildLoggerName("default");
        default_logger_ = builder->Build();
        loggers_.insert(std::make_pair("default",default_logger_));
    }

public:
    static LoggerManager &GetInstance(){
        static LoggerManager eton;
        return eton;
    }

    bool LoggerExist(const std::string &name){
        std::unique_lock<std::mutex> lock(mtx_);
        auto it = loggers_.find(name);
        if(it ==loggers_.end()){
            return false;
        }
        return true;
    }

    void AddLogger(const AsyncLogger::ptr &&AsyncLogger){
        if(LoggerExist(AsyncLogger->Name())){
            return;
        }
        std::unique_lock<std::mutex> lock(mtx_);
        loggers_.insert(std::make_pair(AsyncLogger->Name(),AsyncLogger));
    }

    AsyncLogger::ptr GetLogger(const std::string &name){
        std::unique_lock<std::mutex> lock(mtx_);
        auto it = loggers_.find(name);
        if(it==loggers_.end()){
            // 这里就是返回了一个空指针
            return AsyncLogger::ptr();
        }
        return it->second;
    }

    AsyncLogger::ptr DefaultLogger(){
        return default_logger_;
    }
};
}

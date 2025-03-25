#include "Service.hpp"
#include<thread>

storage::DataManager *data_;
ThreadPool *tp=nullptr;
mylog::Config *g_conf_data;

void service_module(){
    storage::Service s;
    mylog::GetLogger("asynclogger")->Info("service step in RunModule");
    s.RunModule();
}

void log_system_module_inti(){
    g_conf_data = mylog::Config::GetConfig();
    tp = new ThreadPool(g_conf_data->thread_count);
    std::shared_ptr<mylog::LoggerBuilder> slb(new mylog::LoggerBuilder());
    slb->BuildLoggerName("asynclogger");
    slb->BuildLoggerFlush<mylog::RollFileFlush>("./logfile/RollFile_log",1024*1024);
    // 日志管理器已经被构建，并由日志管理器类的成员进行管理。
    // 日志被分配给托管对象，调用者通过调用单例托管对象来记录日志。
    mylog::LoggerManager::GetInstance().AddLogger(slb->Build());
}


int main(){
    log_system_module_inti();
    data_ = new storage::DataManager();
    std::thread t1(service_module);

    t1.join();
    delete tp;
    return 0;
}

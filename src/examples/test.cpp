#include "../../logSystem/logsCode/MyLog.hpp"
#include "../../logSystem/logsCode/ThreadPoll.hpp"
#include "../../logSystem/logsCode/Util.hpp"
#include <chrono>
using std::cout;
using std::endl;

ThreadPool* tp=nullptr;
mylog::Config* g_conf_data;
void test() {
    int cur_size = 0;
    int cnt = 1;
    auto logger = mylog::GetLogger("asynclogger");
    auto start = std::chrono::high_resolution_clock::now();
    while (cur_size++ < 500000) {
        logger->Info("测试日志-%d", cnt++);
        logger->Warn("测试日志-%d", cnt++);
        logger->Debug("测试日志-%d", cnt++);
        // logger->Error("测试日志-%d", cnt++);
        // logger->Fatal("测试日志-%d", cnt++);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "程序运行时间：" << duration.count() << " 微秒" << std::endl;
}

void init_thread_pool() {
    tp = new ThreadPool(g_conf_data->thread_count);
}
int main() {
    g_conf_data = mylog::Config::GetConfig();
    init_thread_pool();
    std::shared_ptr<mylog::LoggerBuilder> Glb(new mylog::LoggerBuilder());
    Glb->BuildLoggerName("asynclogger");
    Glb->BuildLoggerFlush<mylog::FileFlush>("./logfile/FileFlush.log");
    Glb->BuildLoggerFlush<mylog::RollFileFlush>("./logfile/RollFile_log",
                                              1024 * 1024);
    //建造完成后，日志器已经建造，由LoggerManger类成员管理诸多日志器
    // 把日志器给管理对象，调用者通过调用单例管理对象对日志进行落地
    mylog::LoggerManager::GetInstance().AddLogger(Glb->Build());
    test();
    delete(tp);
    return 0;
}
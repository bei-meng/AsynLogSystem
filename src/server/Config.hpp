#pragma once
#include "Util.hpp"
#include<memory>
#include<mutex>
// 该类用于读取配置文件的信息
namespace storage{
    // 懒汉模式
    const char *config_file = "Storage.conf";
    class Config{
    private:
        int server_port_;
        std::string server_ip_;
        std::string download_prefix_;       //URL路径前缀
        std::string deep_storage_dir_;      //深度存储文件的存储路径
        std::string low_storage_dir_;       //浅度存储文件的存储路径
        std::string storage_info_;          //已存储文件的信息
        std::string uncompress_dir_;        //用于解压缩文件夹
        int bundle_format_;                 //深度存储的文件后缀，由选择的压缩格式确定
    
    private:
        static std::mutex mutex_;
        static Config *instance_;
        Config(){
            if(ReadConfig()==false){
                mylog::GetLogger("asynclogger")->Fatal("ReadConfig failed");
                return;
            }
            mylog::GetLogger("asynclogger")->Info("ReadConfig complicate");
        }

        bool ReadConfig(){
            mylog::GetLogger("asynclogger")->Info("ReadConfig start");
            storage::FileUtil fu(config_file);
            std::string content;
            if(!fu.GetContent(content)){
                return false;
            }

            Json::Value root;
            // 反序列化，将内容转成json value格式
            storage::JsonUtil::UnSerialize(content,&root);

            // 转换时要记得用上asint,asstring这种函数，json数据类型为Value
            server_port_ = root["server_port"].asInt();
            server_ip_ = root["server_ip"].asString();
            download_prefix_ = root["download_prefix"].asString();
            storage_info_ = root["storage_info"].asString();
            deep_storage_dir_ = root["deep_storage_dir"].asString();
            low_storage_dir_ = root["low_storage_dir"].asString();
            bundle_format_ = root["bundle_format"].asInt();
            uncompress_dir_ = root["uncompress_dir"].asString();

            return true;
        }

    public:
        int GetServerPort(){ return server_port_; }
        int GetBundleFormat(){return bundle_format_; }
        std::string GetServerIp(){ return server_ip_; }
        std::string GetDownloadPrefix(){return download_prefix_;}
        std::string GetDeepStorageDir(){ return deep_storage_dir_; }
        std::string GetLowStorageDir(){ return low_storage_dir_; }
        std::string GetStorageInfoFile() { return storage_info_; }
        std::string GetUnCompressDir() { return uncompress_dir_; }
    
    public:
        // 获取单例对象
        static Config *GetInstance(){
            if(instance_ == nullptr){
                std::unique_lock<std::mutex> lock(mutex_);
                if(instance_ == nullptr){
                    instance_ = new Config();
                }
            }
            return instance_;
        }
    };
    // 静态成员初始化，先写类型再写类域
    std::mutex Config::mutex_;
    Config *Config::instance_= nullptr;
}
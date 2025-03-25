#pragma once
#include"Config.hpp"
#include<unordered_map>
#include<pthread.h>
namespace storage{
    class StorageInfo{
    public:
        time_t mtime_;//最近修改时间
        time_t atime_;//最近访问时间
        size_t fsize_;
        std::string storage_path_;  //文件存储路径
        std::string url_;           //请求URL中的资源路径

        bool NewStorageInfo(const std::string &storage_path){
            // 初始化备份文件的信息
            mylog::GetLogger("asynclogger")->Debug("NewStorageInfo start");
            FileUtil f(storage_path);
            if(!f.Exists()){
                mylog::GetLogger("asynclogger")->Info("file not exists");
                return false;
            }
            mtime_ = f.LastModifyTime();
            atime_ = f.LastAccessTime();
            fsize_ = f.FileSize();
            storage_path_ = storage_path;

            // URL实际上就是用户下载文件请求的路径
            // 下载路径前缀+文件名
            storage::Config *config = storage::Config::GetInstance();
            url_ = config->GetDownloadPrefix()+storage_path_.substr(2);
            mylog::GetLogger("asynclogger")->Info("download_url:%s,mtime_:%s,atime_:%s,fsize_:%d", url_.c_str(),ctime(&mtime_),ctime(&atime_),fsize_);
            mylog::GetLogger("asynclogger")->Debug("NewStorageInfo end");
            return true;
        }
    };//StorageInfo

    class DataManager{
    private:
        std::string storage_file_;
        pthread_rwlock_t rwlock_;//POSIX线程库中用于控制读写锁的结构体
        std::unordered_map<std::string,StorageInfo> table_;
    public:
        DataManager(){
            mylog::GetLogger("asynclogger")->Debug("DataManager construct start");
            storage_file_ = storage::Config::GetInstance()->GetStorageInfoFile();
            pthread_rwlock_init(&rwlock_,NULL);
            InitLoad();
        }

        ~DataManager(){
            pthread_rwlock_destroy(&rwlock_);
        }

        bool InitLoad(){
            // 初始化程序运行时从文件读取数据
            mylog::GetLogger("asynclogger")->Debug("init datamanager");
            storage::FileUtil f(storage_file_);
            if(!f.Exists()){
                mylog::GetLogger("asynclogger")->Info("there is no storage file info need to load");
                return false;
            }

            std::string body;
            if(!f.GetContent(body)){
                return false;
            }

            // 反序列化
            Json::Value root;
            storage::JsonUtil::UnSerialize(body,&root);
            // 将反序列化得到的Json::Value中的数据添加到table中
            for(auto data:root){
                StorageInfo info;
                info.fsize_ = data["fsize_"].asInt();
                info.atime_ = data["atime_"].asInt();
                info.mtime_ = data["mtime_"].asInt();
                info.storage_path_ = data["storage_path_"].asString();
                info.url_ = data["url_"].asString();
                pthread_rwlock_wrlock(&rwlock_);//加写锁
                table_[info.url_]=info;
                pthread_rwlock_unlock(&rwlock_);;
            }
            return true;
        }

        bool Insert(const StorageInfo &info){
            pthread_rwlock_wrlock(&rwlock_);//加写锁
            table_[info.url_]=info;
            pthread_rwlock_unlock(&rwlock_);
            if(Storage()==false){
                mylog::GetLogger("asynclogger")->Error("data_message Insert:Storage Error");
                return false;
            }
            return true;
        }

        bool Storage(){
            // 每次有信息改变则需要持久化存储一次
            // 把table_中的数据转成json格式存入文件
            std::vector<StorageInfo> arr;
            if(!GetAll(arr)){
                mylog::GetLogger("asynclogger")->Warn("GetAll fail,can't get StorageInfo");
                return false;
            }
            Json::Value root;
            for(auto e:arr){
                Json::Value item;
                item["mtime_"] = (Json::Int64)e.mtime_;
                item["atime_"] = (Json::Int64)e.atime_;
                item["fsize_"] = (Json::Int64)e.fsize_;
                item["url_"] = e.url_.c_str();
                item["storage_path_"] = e.storage_path_.c_str();
                root.append(item);
            }

            std::string body;
            JsonUtil::Serialize(root,body);
            // mylog::GetLogger("asynclogger")->Info("new message for StorageInfo:%s", body.c_str());

            FileUtil f(storage_file_);
            if(f.SetContent(body.c_str(),body.size())==false){
                mylog::GetLogger("asynclogger")->Error("SetContent for StorageInfo Error");
            }
            return true;
        }

        bool GetAll(std::vector<StorageInfo> &arry){
            pthread_rwlock_rdlock(&rwlock_);//获取读锁
            for(auto i:table_){
                arry.emplace_back(i.second);
            }
            pthread_rwlock_unlock(&rwlock_);
            return true;
        }

        bool GetOneByURL(const std::string &key,StorageInfo &info){
            pthread_rwlock_rdlock(&rwlock_);//加读锁
            auto it = table_.find(key);
            if(it==table_.end()){
                return false;
            }

            info = it->second;
            pthread_rwlock_unlock(&rwlock_);
            return true;
        }

        bool GetOneByStoragePath(const std::string &storage_path,StorageInfo &info){
            pthread_rwlock_rdlock(&rwlock_);//加读锁
            for(auto e:table_){
                if(e.second.storage_path_==storage_path){
                    info = e.second;
                    pthread_rwlock_unlock(&rwlock_);
                    return true;
                }
            }
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
    };
}
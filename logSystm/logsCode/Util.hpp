#pragma once
// 该文件定义了文件状态相关的函数和宏
#include<sys/stat.h>
// 该文件定义了系统中常用的类型定义
#include<sys/types.h>
#include<jsoncpp/json/json.h>
#include<ctime>
#include<fstream>
#include <iostream>
#include"jsonConfigPath.h"

using std::cout;
using std::endl;
namespace mylog{
namespace Util{
    // 日期类
    class Date{
    public:
        static time_t Now(){return time(nullptr);}
    };//class Date

    // 
    class File{
    public:
        // 判断文件是否存在
        static bool Exists(const std::string &filename){
            struct stat st;
            // 返回0表示获取文件信息成功，-1失败
            return (0==stat(filename.c_str(),&st));
        }

        // 返回文件所在路径
        static std::string Path(const std::string &filename){
            if(filename.empty()){
                return "";
            }
            // 寻找最后一个/或者反斜杠
            int pos = filename.find_last_of("/\\");
            // 返回这个文件所在路径
            if(pos != std::string::npos){
                return filename.substr(0,pos+1);
            }
            return "";
        }//Path

        // 创建文件夹
        static void CreateDirectory(const std::string &pathname){
            if(pathname.empty()){
                perror("path is empty");
            }
            // 路径不存在再创建
            if(!Exists(pathname)){
                size_t pos,index = 0;
                size_t size = pathname.size();
                while(index<size){
                    pos = pathname.find_first_of("/\\",index);
                    // 没有更深一层目录了
                    if(pos==std::string::npos){
                        // 八进制数设置创建目录的权限
                        if (mkdir(pathname.c_str(), 0755) == -1) {
                            std::cerr << __FILE__ << __LINE__ << ":";
                            perror("create directory error:");
                        }
                        return;
                    }
                    if(pos==index){
                        index = pos+1;
                        continue;
                    }
                    // 否则切分目录
                    std::string subPath = pathname.substr(0,pos);
                    // 判断切分出的目录是否存在
                    if(subPath=="."||subPath==".."||Exists(subPath)){
                        index = pos +1;
                        continue;
                    }
                    // 切分出的目录不是当前目录或父目录，且不存在
                    if (mkdir(subPath.c_str(), 0755) == -1) {
                        std::cerr << __FILE__ << __LINE__ << ":";
                        perror("create directory error:");
                        return;
                    }
                    index = pos + 1;
                }
            }
        }//CreateDirectory

        // 获取文件大小
        static int64_t FileSize(const std::string &filename){
            struct stat s;
            auto res = stat(filename.c_str(),&s);
            if(res==-1){
                std::cerr << __FILE__ << __LINE__ << ":";
                perror("get file size error:");
                return -1;
            }
            return s.st_size;
        }//FileSize

        // 获取文件内容
        static bool GetContent(std::string &content,const std::string &filename){
            // 打开文件
            std::ifstream ifs;
            ifs.open(filename.c_str(),std::ios::binary);
            if(ifs.is_open()==false){
                std::cerr << __FILE__ << __LINE__ << ":";
                perror("open file error: ");
                return false;
            }
            // 文件指针移动到开头
            ifs.seekg(0,std::ios::beg);
            int64_t len = FileSize(filename);
            if(len==-1){
                return false;
            }
            content.resize(len);
            // 读取文件
            ifs.read(&content[0],len);
            if(!ifs.good()){
                std::cerr << __FILE__ << __LINE__ << ":";
                perror("read file content error: ");
                ifs.close();
                return false;
            }
            ifs.close();
            return true;
        }
    };//class file
    
    class JsonUtil{
    public:
        // 序列化
        static bool Serialize(const Json::Value &val,std::string &str){
            // 建造者生成->建造者实例化json写对象->调用写对象中的接口进行序列化写入str
            Json::StreamWriterBuilder swb;
            std::unique_ptr<Json::StreamWriter> usw(swb.newStreamWriter());
            std::stringstream ss;
            if(usw->write(val,&ss)!=0){
                std::cerr << __FILE__ << __LINE__ << ":";
                perror("serialize error: ");
                return false;
            }
            str = std::move(ss.str());
            return true;
        }

        // 反序列化
        static bool UnSerialize(const std::string &str,Json::Value *val){
            Json::CharReaderBuilder crb;
            std::unique_ptr<Json::CharReader> ucr(crb.newCharReader());
            std::string err;
            if(ucr->parse(str.c_str(),str.c_str()+str.size(),val,&err)==false){
                std::cerr <<__FILE__<<__LINE__<<"parse error" << err<<std::endl;
                return false;
            }
            return true;
        }
    };//JsonUtil

    struct JsonData{
    private:
        JsonData(){
            std::string content;
            const std::string& path = JsonConfigPath::GetConfigPath();
            if(mylog::Util::File::GetContent(content,path)==false){
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
        static JsonData* GetJsonData(){
            // C++11保证了局部静态变量的初始化是线程安全的。
            // 避免指针构造
            static JsonData jsonData;
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
}// namespace Util
}// namespace mylog
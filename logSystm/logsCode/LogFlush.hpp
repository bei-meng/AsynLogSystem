// 提供断言功能,宏在调试模式下有效（默认情况下），但在发布模式下会禁用
#include<cassert>
// 文件流
#include<fstream>
// 智能指针和内存管理工具
#include<memory>
// 提供 POSIX 标准的系统调用接口
#include<unistd.h>

#include "Util.hpp"
// 单例模式创建得到的JsonData数据
extern mylog::Util::JsonData* g_conf_data;
namespace mylog{
    // 基类LogFlush
    class LogFlush{
    public:
        using ptr = std::shared_ptr<LogFlush>;
        virtual ~LogFlush(){}
        // 纯虚函数，派生类必须实现
        virtual void Flush(const char *data,size_t len) = 0;
    };//LogFlush

    //标准输出
    class StdoutFlush:public LogFlush{
    public:
        using ptr = std::shared_ptr<StdoutFlush>;
        void Flush(const char *data,size_t len) override{
            std::cout.write(data,len);
        }
    };

    // 文件刷入缓存或磁盘
    class FileFlush:public LogFlush{
    private:
        std::string filename_;
        FILE* fs_ = NULL; 
    public:
        using ptr = std::shared_ptr<FileFlush>;
        FileFlush(const std::string &filename):filename_(filename){
            // 创建所给目录
            Util::File::CreateDirectory(Util::File::Path(filename));
            // a追加模式，b二进制模式
            fs_ = fopen(filename.c_str(),"ab");
            if(fs_ == NULL){
                std::cerr <<__FILE__<<__LINE__<<"open log file failed"<< std::endl;
                perror(NULL);
            }
        }
        void Flush(const char *data,size_t len) override{
            fwrite(data,1,len,fs_);
            if(ferror(fs_)){
                std::cerr <<__FILE__<<__LINE__<<"write log file failed"<< std::endl;
                perror(NULL);
            }
            if(g_conf_data->flush_log==1){
                if(fflush(fs_)==EOF){
                    std::cerr <<__FILE__<<__LINE__<<"fflush file failed"<< std::endl;
                    perror(NULL);
                }
            }else if(g_conf_data->flush_log==2){
                    // 刷新文件缓冲区
                if (fflush(fs_) == EOF) {
                    std::cerr << __FILE__ << ":" << __LINE__ << ": fflush file failed" << std::endl;
                    perror(NULL);
                }

                // 同步文件到磁盘,fileno获取文件流的文件描述符
                if (fsync(fileno(fs_)) == -1) {
                    std::cerr << __FILE__ << ":" << __LINE__ << ": fsync file failed" << std::endl;
                    perror(NULL);
                }
            }
        }
    };//FileFlush

    // 滚动日志文件，每个日志文件大小有限制
    class RollFileFlush:public LogFlush{
    private:
        size_t cnt_ = 1;//文件计数器
        size_t cur_size_ = 0;//当前文件大小
        size_t max_size_;//每个文件的最大存放大小
        std::string basename_;//基础文件名
        // std::ofstream ofs_;
        FILE* fs_ = NULL;

        // 构建最后的滚动日志文件名称
        std::string CreateFilename(){
            time_t time = Util::Date::Now();
            struct tm t;
            localtime_r(&time,&t);
            std::string filename = basename_;
            filename += std::to_string(t.tm_year+1900);
            filename += std::to_string(t.tm_mon+1);
            filename += std::to_string(t.tm_mday);
            filename += std::to_string(t.tm_hour);
            filename += std::to_string(t.tm_min);
            filename += std::to_string(t.tm_sec);
            filename += '-' + std::to_string(cnt_++) + ".log";
            return filename;
        }

        // 初始化文件流，每次文件到达最大值或者初始化时会执行
        void InitLogFile(){
            if(fs_==NULL || cur_size_>=max_size_){
                if(fs_!=NULL){
                    fclose(fs_);
                    fs_=NULL;
                }
                std::string filename = CreateFilename();
                fs_ = fopen(filename.c_str(),"ab");
                if(fs_==NULL){
                    std::cerr <<__FILE__<<__LINE__<<"open file failed"<< std::endl;
                    perror(NULL);
                }
                cur_size_ = 0;
            }
        }
    public:
        using ptr = std::shared_ptr<RollFileFlush>;
        // 初始化时创建基础文件路径
        RollFileFlush(const std::string &filename,size_t max_size)
        :max_size_(max_size),basename_(filename){
            Util::File::CreateDirectory(Util::File::Path(filename));
        }

        void Flush(const char *data,size_t len)override{
            // 确认文件大小，是否需要新建log文件
            InitLogFile();
            // 写入数据
            fwrite(data,1,len,fs_);
            if(ferror(fs_)){
                std::cerr <<__FILE__<<__LINE__<<"write log file failed"<< std::endl;
                perror(NULL);
            }
            cur_size_ += len;
            if(g_conf_data->flush_log==1){
                if(fflush(fs_)==EOF){
                    std::cerr <<__FILE__<<__LINE__<<"fflush file failed"<< std::endl;
                    perror(NULL);
                }
            }else if(g_conf_data->flush_log==2){
                    // 刷新文件缓冲区
                if (fflush(fs_) == EOF) {
                    std::cerr << __FILE__ << ":" << __LINE__ << ": fflush file failed" << std::endl;
                    perror(NULL);
                }

                // 同步文件到磁盘,fileno获取文件流的文件描述符
                if (fsync(fileno(fs_)) == -1) {
                    std::cerr << __FILE__ << ":" << __LINE__ << ": fsync file failed" << std::endl;
                    perror(NULL);
                }
            }
        }
    };//RollFileFlush

    class LogFlushFactory{
    public:
        using ptr = std::shared_ptr<LogFlushFactory>;
        // 这里使用建造者模式，在建造者内部封装好了日志器的创建过程，确保建造的日志器是合规的
        // 其次建造者模式允许不改变现有日志器的基础上，通过在建造过程中集成新的功能组件来扩展日志器的功能
        template <typename FlushType,typename ...Args>
        static std::shared_ptr<LogFlush> CreateLog(Args &&...args){
            return std::make_shared<FlushType>(std::forward<Args>(args)...);
        }
    };
}

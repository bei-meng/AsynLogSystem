#pragma once
#include "DataManager.hpp"
// 提供了一组宏，用于实现各种类型的队列（如链表、栈、队列等）。
// 用途：常用于系统编程和内核编程中，用于管理数据结构
#include <sys/queue.h>

// libevent 是一个事件通知库，用于处理异步事件。
// 用途：用于网络编程和事件驱动的编程，支持多种事件类型（如信号、定时器、I/O 事件等）
#include<event.h>

// for HTTP
// libevent 提供的 HTTP 服务器和客户端功能。
// 用途：用于构建 HTTP 服务器或客户端，处理 HTTP 请求和响应。
#include<evhttp.h>
#include<event2/http.h>


// <fcntl.h>：提供文件控制操作，如打开、关闭文件描述符，设置文件标志等。
// <sys/stat.h>：提供文件状态信息，如文件的大小、权限、类型等
#include<fcntl.h>
#include<sys/stat.h>

// C++ 标准库中的正则表达式库
#include<regex>
#include"base64.h"//来自cpp-base64库

extern storage::DataManager *data_;

namespace storage{
    class Service{
    private:
        uint16_t server_port_;
        std::string server_ip_;
        std::string download_prefix_;
    public:
        Service(){
            mylog::GetLogger("asynclogger")->Debug("Service start(Construct)");
            server_port_ = Config::GetInstance()->GetServerPort();
            server_ip_ = Config::GetInstance()->GetServerIp();
            download_prefix_ = Config::GetInstance()->GetDownloadPrefix();
        }
        
        bool RunModule(){
            // 初始化事件循环
            event_base *base = event_base_new();
            if(base==NULL){
                mylog::GetLogger("asynclogger")->Fatal("event_base_new err!");
                return false;
            }
            // 设置监听的端口和地址
            sockaddr_in sin;
            memset(&sin,0,sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_port = htons(server_port_);
            // http服务器，创建evhttp上下文
            evhttp *httpd = evhttp_new(base);
            // 绑定端口和ip
            // 监听所有可用的网络接口,可以确保服务能够接受来自本地网络、外部网络以及本机内部的连接。
            if(evhttp_bind_socket(httpd,"0.0.0.0",server_port_)!=0){
                mylog::GetLogger("asynclogger")->Fatal("evhttp_bind_socket failed!");
                return false;
            }
            // 设定回调函数
            // 指定通用的回调函数，也可以为特定的URI指定callback
            evhttp_set_gencb(httpd,GenHandler,NULL);
            if(base){
                if(event_base_dispatch(base)==-1){
                    mylog::GetLogger("asynclogger")->Fatal("event_base_dispatch err");
                }
            }
            if(base){
                event_base_free(base);
            }
            if(httpd){
                evhttp_free(httpd);
            }
            return true;
        }

    private:
        static void GenHandler(struct evhttp_request *req,void *arg){
            // evhttp_request_get_evhttp_uri从http请求中获取uri
            // evhttp_uri_get_path从uri中获取路径
            std::string path = evhttp_uri_get_path(evhttp_request_get_evhttp_uri(req)); 
            path = UrlDecode(path);
            mylog::GetLogger("asynclogger")->Info("get req, uri:%s",path.c_str());
            // 这里根据请求中的内容判断是什么请求
            // 下载请求
            if(path.find("/download/")!=std::string::npos){
                Download(req,arg);
            }else if(path == "/upload"){
                Upload(req,arg);
            }else if(path == "/"){
                ListShow(req,arg);
            }else{
                evhttp_send_reply(req,HTTP_NOTFOUND,"Not Found",NULL);
            }
        }

        static void Download(struct evhttp_request *req,void *arg){
            mylog::GetLogger("asynclogger")->Info("Download");
            // 获取客户端请求的资源路径path，req.path
            // 1.根据资源路径，获取StorageInfo
            StorageInfo info;
            std::string resource_path = evhttp_uri_get_path(evhttp_request_get_evhttp_uri(req));
            resource_path = UrlDecode(resource_path);
            data_->GetOneByURL(resource_path,info);
            mylog::GetLogger("asynclogger")->Info("request resource_path:%s", resource_path.c_str());
            
            
            // 2.判断文件是否存在
            std::string download_path = info.storage_path_;
            FileUtil f(download_path);
            if (f.Exists() == false){
                mylog::GetLogger("asynclogger")->Info("%s not exists", download_path.c_str());
                download_path += "not exists";
                evhttp_send_reply(req, 404, download_path.c_str(), NULL);
                return;
            }

            // 3.解压缩文件，如果压缩过就解压到新文件夹给用户下载
            if(download_path.find(Config::GetInstance()->GetLowStorageDir())==std::string::npos){
                mylog::GetLogger("asynclogger")->Info("uncompressing:%s", download_path.c_str());
                download_path = Config::GetInstance()->GetUnCompressDir()+
                std::string(download_path.begin() + download_path.find_last_of('/') + 1, download_path.end());
                FileUtil dirCreate(Config::GetInstance()->GetUnCompressDir());
                dirCreate.CreateDirectory();
                f.UnCompress(download_path);
            }

            // 
            mylog::GetLogger("asynclogger")->Info("request download_path:%s", download_path.c_str());
            FileUtil fu(download_path);
            if(fu.Exists()==false){
                if(info.storage_path_.find("deep_storage")!=std::string::npos){
                    // 如果是压缩文件，且解压失败，是服务端的错误
                    mylog::GetLogger("asynclogger")->Info("evhttp_send_reply: 500 - UnCompress failed");
                    evhttp_send_reply(req, HTTP_INTERNAL, NULL, NULL);
                }
                return;
            }

            // 断点续传未实现

            // 读取文件数据，放入rsp.body中
            evbuffer *outbuf = evhttp_request_get_output_buffer(req);
            int fd = open(download_path.c_str(),O_RDONLY);
            if(fd == -1){
                mylog::GetLogger("asynclogger")->Error("open file error: %s -- %s", download_path.c_str(), strerror(errno));
                evhttp_send_reply(req, HTTP_INTERNAL, strerror(errno), NULL);
                return;
            }

            // 和evbuffer_add类似，但是效率更高，具体原因可以看函数声明
            if(evbuffer_add_file(outbuf, fd, 0, fu.FileSize()) == -1){
                mylog::GetLogger("asynclogger")->Error("evbuffer_add_file: %d -- %s -- %s", fd, download_path.c_str(), strerror(errno));
            }

            // 5. 设置响应头部字段： ETag， Accept-Ranges: bytes
            evhttp_add_header(req->output_headers, "Accept-Ranges", "bytes");
            evhttp_add_header(req->output_headers, "ETag", GetETag(info).c_str());
            evhttp_add_header(req->output_headers, "Content-Type", "application/octet-stream");

            evhttp_send_reply(req, HTTP_OK, "Success", NULL);
            mylog::GetLogger("asynclogger")->Info("evhttp_send_reply: HTTP_OK");

            //说明是解压缩的文件
            if (download_path != info.storage_path_){
                remove(download_path.c_str()); // 删除文件
            }
        }

        static void Upload(struct evhttp_request *req,void *arg){
            mylog::GetLogger("asynclogger")->Info("Upload start");
            // 约定：请求中包含"low_storage"，说明请求中存在文件数据,并希望普通存储\
                包含"deep_storage"字段则压缩后存储
            struct evbuffer *buf = evhttp_request_get_input_buffer(req);
            if(buf == nullptr){
                mylog::GetLogger("asynclogger")->Info("evhttp_request_get_input_buffer is empty");
                return;
            }

            // 获取请求体的长度
            size_t len = evbuffer_get_length(buf);
            mylog::GetLogger("asynclogger")->Info("evbuffer_get_length is %u", len);
            if(len == 0){
                evhttp_send_reply(req, HTTP_BADREQUEST, "file empty", NULL);
                mylog::GetLogger("asynclogger")->Info("request body is empty");
                return;
            }
            // 获取文件数据
            std::string content(len, 0);
            if(evbuffer_copyout(buf,(void*)content.c_str(),len)==-1){
                mylog::GetLogger("asynclogger")->Error("evbuffer_copyout error");
                evhttp_send_reply(req,HTTP_INTERNAL,NULL,NULL);
            }
            // 获取文件名,解决中文文件名的问题，使用base64加密
            std::string filename = evhttp_find_header(req->input_headers,"FileName");
            filename = base64_decode(filename);
            // 获取文件名
            std::string storage_type = evhttp_find_header(req->input_headers,"StorageType");
            // 组织存储路径
            std::string storage_path;
            if(storage_type=="low"){
                storage_path = Config::GetInstance()->GetLowStorageDir();
            }else if(storage_type == "deep"){
                storage_path = Config::GetInstance()->GetDeepStorageDir();
            }else{
                mylog::GetLogger("asynclogger")->Info("evhttp_send_reply: HTTP_BADREQUEST");
                evhttp_send_reply(req, HTTP_BADREQUEST, "Illegal storage type", NULL);
                return;
            }
            // 如果路径不存在就创建路径
            FileUtil dirCreate(storage_path);
            dirCreate.CreateDirectory();

            // 目录创建后可以加上文件名，这个就是要最终要写入的文件路径
            storage_path += filename;
            mylog::GetLogger("asynclogger")->Debug("storage_path:%s", storage_path.c_str());

            FileUtil fu(storage_path);
            if(storage_path.find("low_storage")!=std::string::npos){
                if(fu.SetContent(content.c_str(),len)==false){
                    mylog::GetLogger("asynclogger")->Error("low_storage fail, evhttp_send_reply: HTTP_INTERNAL");
                    evhttp_send_reply(req, HTTP_INTERNAL, "server error", NULL);
                    return;
                }else{
                    mylog::GetLogger("asynclogger")->Info("low_storage success");
                }
            }else{
                if(fu.Compress(content,Config::GetInstance()->GetBundleFormat())==false){
                    mylog::GetLogger("asynclogger")->Error("deep_storage fail, evhttp_send_reply: HTTP_INTERNAL");
                    evhttp_send_reply(req,HTTP_INTERNAL,"server error",NULL);
                    return;
                }else{
                    mylog::GetLogger("asynclogger")->Info("deep_storage success");
                }
            }

            // 添加文件信息，交由数据管理类进行管理
            StorageInfo info;
            // 组织文件存储的信息
            info.NewStorageInfo(storage_path);
            // 向数据管理模块添加存储的文件信息
            data_->Insert(info);

            evhttp_send_reply(req, HTTP_OK, "Success", NULL);
            mylog::GetLogger("asynclogger")->Info("upload finish:success");
        }

        static void ListShow(struct evhttp_request *req,void *arg){
            mylog::GetLogger("asynclogger")->Info("ListShow()");
            // 获取所有的文件存储信息
            std::vector<StorageInfo> arry;
            data_->GetAll(arry);

            // 读取模板文件
            std::ifstream templateFile("index.html");
            if(!templateFile.is_open()){
                mylog::GetLogger("asynclogger")->Fatal("Failed to open file: index.html");
                return;
            }
            // 避免解释为函数声明，需要加括号
            std::string templateContent(
                (std::istreambuf_iterator<char>(templateFile)),
                std::istreambuf_iterator<char>()
            );

            // 替换html中的占位符
            // 替换文件列表进html
            templateContent = std::regex_replace(templateContent,
                                                std::regex("\\{\\{FILE_LIST\\}\\}"),
                                                generateModernFileList(arry));

            // 替换html文件中的占位符
            templateContent = std::regex_replace(templateContent,
                                                std::regex("\\{\\{BACKEND_URL\\}\\}"),
                                                "http://"+storage::Config::GetInstance()->GetServerIp()+":"+std::to_string(storage::Config::GetInstance()->GetServerPort()));
            
            struct evbuffer *buf = evhttp_request_get_output_buffer(req);
            // auto response_body = templateContent
            // 把前面的html数据给到evbuffer，然后设置响应头部字段，最后返回给服务器
            evbuffer_add(buf,(const void *)templateContent.c_str(),templateContent.size());
            evhttp_add_header(req->output_headers,"Content-Type","text/html;charset=utf-8");
            evhttp_send_reply(req,HTTP_OK,NULL,NULL);
            mylog::GetLogger("asynclogger")->Info("ListShow() finish");
        }

        static std::string generateModernFileList(const std::vector<StorageInfo> &files){
            std::stringstream ss;
            ss<<"<div class='file-list'><h3>已上传文件</h3>";
            for(const auto &file:files){
                std::string filename = FileUtil(file.storage_path_).FileName();
                std::string storage_type = "low";
                if(file.storage_path_.find("/deep_storage/")!=std::string::npos){
                    storage_type = "deep";
                }
                ss  << "<div class='file-item'>"
                        << "<div class='file-info'>"
                            << "<span>📄" <<filename<<"</span>"
                            << "<span class='file-type'>"
                            << (storage_type == "deep"? "压缩存储":"普通存储")
                            << "</span>"
                            << "<span>" <<FormatSize(file.fsize_)<<"</span>"
                            << "<span>" <<TimeToStr(file.mtime_)<<"</span>"
                        <<"</div>"
                        << "<button onclick=\"window.location='"<<file.url_<<"'\">⬇️ 下载</button>"
                    << "</div>";
            }
            ss<<"</div>";
            return ss.str();
        }

        static std::string FormatSize(uint64_t bytes){
            const char *units[]={"B","KB","MB","GB"};
            int unit_index = 0;
            double size = bytes;
            while(size>1024 && unit_index<3){
                size /= 1024;
                ++unit_index;
            }
            std::stringstream ss;
            ss<<std::fixed<<std::setprecision(2)<<size<<" "<<units[unit_index];
            return ss.str();
        }

        static std::string TimeToStr(time_t t){
            return std::ctime(&t);
        }
        static std::string GetETag(const StorageInfo &info){
            // 自定义etag
            FileUtil fu(info.storage_path_);
            std::string etag = fu.FileName();
            etag += "-";
            etag += std::to_string(info.fsize_);
            etag += "-";
            etag += std::to_string(info.mtime_);
            return etag;
        }
    };
}
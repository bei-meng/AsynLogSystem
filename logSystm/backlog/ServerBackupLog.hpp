#pragma once
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>

using std::cout;
using std::endl;
using func_t = std::function<void(const std::string &)>;
const int backlog = 32;//最大连接数量

// 请求响应模型，每次最多处理1024字节，处理完成就断开TCP连接
class TcpServer;
class ThreadData{
public:
    int sock;
    std::string client_ip;
    uint16_t client_port;
    TcpServer *ts_;
public:
    ThreadData(int fd,const std::string &ip,const uint16_t &port,TcpServer *ts)
    :sock(fd),client_ip(ip),client_port(port),ts_(ts){}
};

class TcpServer{
private:
    int listen_sock_;
    uint16_t port_;
    func_t func_;
public:
    TcpServer(uint16_t port,func_t func):port_(port),func_(func){}

    void init_service(){
        listen_sock_ = socket(AF_INET,SOCK_STREAM,0);
        if(listen_sock_==-1){
            std::cerr << __FILE__ << __LINE__ <<"create socket error"<< strerror(errno)<< std::endl;
        }

        struct sockaddr_in local;
        memset(&local,0,sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = htons(port_);
        local.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(listen_sock_, (struct sockaddr *)&local, sizeof(local))<0){
            std::cerr << __FILE__ << __LINE__ << "bind socket error"<< strerror(errno)<< std::endl;
        }

        if(listen(listen_sock_,backlog)<0){
            std::cerr << __FILE__ << __LINE__ <<  "listen error"<< strerror(errno)<< std::endl;
        }
    }

    //这里最好加个互斥锁，保证多线程写文件的正确性
    void service(int sock,const std::string&& client_info){
        char buff[1024];
        int r_ret = read(sock,buff,sizeof(buff));
        if(r_ret==-1){
            std::cerr << __FILE__ << __LINE__ <<"read error"<< strerror(errno)<< std::endl;
            perror("NULL");
        }else if(r_ret>0){
            buff[r_ret] = 0;
            std::string tmp = buff;
            // 函数回调处理信息
            func_(client_info+tmp);
        }
    }

    void start_service(){
        while(true){
            struct sockaddr_in client_addr;
            socklen_t client_addrlen = sizeof(client_addr);
            int connfd = accept(listen_sock_,(struct sockaddr *)&client_addr,&client_addrlen);
            
            if(connfd<0){
                std::cerr << __FILE__ << __LINE__ << "accept error"<< strerror(errno)<< std::endl;
                continue;
            }

            // 获取客户端信息
            std::string client_ip = inet_ntoa(client_addr.sin_addr);//网络字节序转字符串
            uint16_t client_port = ntohs(client_addr.sin_port);

            // 多个线程提供服务，传入线程数据类型来访问threadRoutine,
            // 因为该函数时static的，所以内部传入了data类型存了tcpserver类型
            pthread_t tid;
            ThreadData *td = new ThreadData(connfd,client_ip,client_port,this);
            pthread_create(&tid,nullptr,threadRoutine,td); 
        }
    }

    static void *threadRoutine(void *args){
        // 调用pthread_detach函数，将当前线程设置为分离状态。
        // 分离线程后，线程在结束时会自动释放其占用的资源，而不需要主线程调用pthread_join来等待线程结束并回收资源。
        // 防止主线程在start_service函数中因等待线程结束而阻塞。
        pthread_detach(pthread_self());//
        ThreadData *td = static_cast<ThreadData *>(args);
        std::string client_info = td->client_ip + ":" +std::to_string(td->client_port);
        td->ts_->service(td->sock,std::move(client_info));
        close(td->sock);
        delete td;
        return nullptr;
    }
};
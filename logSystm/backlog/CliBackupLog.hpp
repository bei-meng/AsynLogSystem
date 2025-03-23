// 远程备份debug等级以上的日志信息-发送端
#include<iostream>
#include<cstring>
#include<string>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
// 提供与网络地址转换相关的函数和宏
#include<arpa/inet.h>
// 定义了与网络编程相关的数据结构和常量，主要用于套接字编程
#include<netinet/in.h>
// 提供与系统调用相关的函数和常量，主要用于文件操作、进程控制和环境信息
#include<unistd.h>

#include "../Util.hpp"
extern mylog::Util::JsonData *g_conf_data;
void start_backup(const std::string &message){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock<0){
        std::cerr << __FILE__ << __LINE__ << "socket error : " << strerror(errno) << std::endl;
        perror(NULL);
    }

    struct sockaddr_in server;
    memset(&server,0,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(g_conf_data->backup_port);
    inet_aton(g_conf_data->backup_addr.c_str(),&(server.sin_addr));

    int cnt =5;
    while(-1==connect(sock,(struct sockaddr*)&server,sizeof(server))){
        std::cout<<"reconnet,time="<<cnt--<<std::endl;
        if(cnt<=0){
            std::cerr << __FILE__ << __LINE__ << "connect error : " << strerror(errno) << std::endl;
            close(sock);
            perror(NULL);
            return;
        }
    }

    // 连接成功
    char buffer[1024];
    if(-1==write(sock,message.c_str(),message.size())){
        std::cerr << __FILE__ << __LINE__ << "send to server error : " << strerror(errno) << std::endl;
        perror(NULL);
    }
    close(sock);
}
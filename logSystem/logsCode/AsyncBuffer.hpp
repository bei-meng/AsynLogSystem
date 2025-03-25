/*日志缓冲区类设计*/
#pragma once
#include <cassert>
#include <string>
#include <vector>
#include "Util.hpp"
#include"Config.hpp"

extern mylog::Config* g_conf_data;
namespace mylog{
    class Buffer{
    protected:
        std::vector<char> buffer_;  //缓冲区
        size_t write_pos_;          //生产者此时的位置
        size_t read_pos_;           //消费者此时的位置

        void ToBeEnough(size_t len){
            // 原来的实现可能不足够
            size_t current_size = buffer_.size();
            size_t writeable_size = WriteableSize();
            if(len>=writeable_size){
                size_t new_size = current_size;
                // 如果当前大小小于阈值，加倍扩容
                if (current_size < g_conf_data->threshold) {
                    new_size = 2 * current_size;
                } else {
                    // 如果当前大小大于或等于阈值，增加线性增长值
                    new_size = current_size + g_conf_data->linear_growth;
                }

                // 确保新大小足够容纳所需的数据
                if (new_size < current_size + len) {
                    new_size = current_size + len;
                }

                buffer_.resize(new_size);
            }
        }
    public:
        // 初始化缓冲区大小
        Buffer():write_pos_(0),read_pos_(0){
            buffer_.resize(g_conf_data->buffer_size);
        }
        // 数据写入缓冲区
        void Push(const char *data,size_t len){
            ToBeEnough(len);
            std::copy(data,data+len,&buffer_[write_pos_]);
            write_pos_ +=len;
        }
        char *ReadBegin(int len){
            assert(len<=ReadableSize());
            return &buffer_[read_pos_];
        }

        bool IsEmpty(){
            return write_pos_==read_pos_;
        }

        // 直接交换
        void Swap(Buffer &buf){
            buffer_.swap(buf.buffer_);
            std::swap(read_pos_, buf.read_pos_);
            std::swap(write_pos_, buf.write_pos_);
        }

        // 可写空间
        size_t WriteableSize(){
            return buffer_.size()-write_pos_;
        }
        // 可读空间
        size_t ReadableSize(){
            return write_pos_ - read_pos_;
        }

        const char *Begin(){
            return &buffer_[read_pos_];
        }
        
        void MoveWritePos(size_t len){
            assert(len<=WriteableSize());
            write_pos_+=len;
        }

        void MoveReadPos(size_t len){
            assert(len<=ReadableSize());
            read_pos_ +=len;
        }

        void Reset(){
            write_pos_ = 0;
            read_pos_ = 0;
        }
    };
}
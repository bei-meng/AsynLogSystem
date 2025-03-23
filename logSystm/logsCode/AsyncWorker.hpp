#pragma once
// 提供原子操作，用于多线程环境中的同步和并发控制
#include<atomic>
// 条件变量，用于线程间的同步
#include<condition_variable>
// 提供函数对象和绑定器的实现
#include<functional>

#include<iostream>
// 提供互斥锁的实现，用于线程间的同步
#include<mutex>
// 线程支持，创建和管理线程
#include<thread>

#include "AsyncBuffer.hpp"

namespace mylog{
enum class AsyncType {ASYNC_SAFE,ASYNC_UNSAFE};
using functor = std::function<void(Buffer&)>;
class AsyncWorker{
private:
    AsyncType async_type_;
    // 确保多stop_的读写操作是原子的，避免多线程环境中的竞态条件
    std::atomic<bool> stop_;//用于控制异步工作器的启动
    std::mutex mtx_;//互斥锁
    mylog::Buffer buffer_productor_;//生产者
    mylog::Buffer buffer_consumer_;//消费者
    std::condition_variable cond_productor_;
    std::condition_variable cond_consumer_;
    std::thread thread_;

    functor callback_;//回调函数，用来告知工作器如何落地

    void ThreadEntry(){
        // 消费者线程进入无限循环
        while(1){
            {
                //缓冲区交换完就解锁，让productor继续写入数据
                std::unique_lock<std::mutex> lock(mtx_);
                // 有数据则交换，无数据就阻塞
                // if(buffer_productor_.IsEmpty()){
                    // 只有当pred条件为false时调用该wait函数才会阻塞当前线程，
                    // 并且在收到其它线程的通知后只有当pred为true时才会被解除阻塞
                    // 即没有停止，且生产者为空时，false条件变量就会阻塞当前线程，同时释放锁，等待通知
                    // 当停止或者不为空，true且收到通知，条件变量才解除阻塞,并获取锁
                    // 等待生产者缓冲区不为空或停止
                    cond_consumer_.wait(lock,[&](){
                        return stop_ || !buffer_productor_.IsEmpty();
                    });
                // }
                // 下面一直都获得了锁，有锁交换不会出问题，因为生产者被阻塞了
                // 生产者消费者缓冲区交换
                buffer_consumer_.Swap(buffer_productor_);
                // 固定容量的缓冲区才需要唤醒，但此时锁还这消费者这里
                if(async_type_ == AsyncType::ASYNC_SAFE){
                    cond_productor_.notify_one();
                }
            }//超出作用域，自动释放锁
            

            // 调用回调函数对缓冲区中数据进行处理
            if(!buffer_consumer_.IsEmpty()){
                callback_(buffer_consumer_);
                buffer_consumer_.Reset();
            }

            // 只有当停止且生产者为空才返回
            if(stop_ && buffer_productor_.IsEmpty()){
                return;
            }
        }
    }

public:
    using ptr = std::shared_ptr<AsyncWorker>;
    // AsyncWorker::ThreadEntry默认需要传递this指针
    // 使用子线程异步处理日志
    AsyncWorker(const functor &cb,AsyncType async_type = AsyncType::ASYNC_SAFE)
    :   async_type_(async_type),
        callback_(cb),
        stop_(false),
        thread_(std::thread(&AsyncWorker::ThreadEntry,this)){}
    
    ~AsyncWorker(){
        Stop();
    }

    void Stop(){
        stop_ = true;
        // 所有线程把缓冲区内数据处理完成就结束了
        cond_consumer_.notify_all();
        // 等待线程完成其执行。调用 join 时，调用线程（通常是主线程）会阻塞，
        // 直到被 join 的线程（子线程）完成其执行。
        thread_.join();
    }

    void Push(const char* data,size_t len){
        // 如果数据超过缓冲区最大，就会导致问题，应该改成写一部分，消费一部分
        // 如果生产者队列不足以写下len长度数据，并且缓冲区是固定大小，那么阻塞
        std::unique_lock<std::mutex> lock(mtx_);
        if(AsyncType::ASYNC_SAFE == async_type_){
            cond_productor_.wait(lock,[&]{
                return len<=buffer_productor_.WriteableSize();
            });
        }
        buffer_productor_.Push(data,len);
        cond_consumer_.notify_one();
    }
};
}
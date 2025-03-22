### 基于异步日志系统的云存储服务

### 1.logSystm/logsCode/Util.hpp
- Date:日期，获取当前日期
- File:判断文件是否存在，获取文件路径，创建文件路径，获取文件大小，获取文件内容
- JsonUtil: 序列化和反序列化字符串内容
    - 序列化，建造者生成->建造者实例化json写对象->调用写对象中的接口进行序列化写入
    - 反序列化类似，建造者生成->建造者实例化json读对象->调用读对象中的接口进行序列化解析
- JsonData: 单例模式，解析JsonData数据存储的相关配置，通过私有化构造函数，同时使用局部静态变量获取对象
    - config的地址也使用单例模式，静态成员初始化

### 2.logSystm/logsCode/LogFlush.h
- LogFlush虚基类，作为后面多种日志刷新类的基类，派生类必须实现Flush函数
- StdoutFlush类，将日志刷到标准输出
- FileFlush类，文件刷入文件，根据flush_log来决定何时刷入磁盘，0写入用户缓存，1写入内核缓存，2刷到磁盘
- RollFileFlush类，滚动日志文件，一个日志文件超过设定大小就会自动创建新的log文件，同样受到flush_log控制
- LogFlushFactory类，建造者模式，日志刷新工厂类，使用模板，完美转发实现对应日志类的构建，易扩展

### 2.logSystm/logsCode/AsyncBuffer.h
- 异步缓冲区模块，业务需要写一条日志的话，会被存放到AsyncBuffer里面，生产者消费者模型，使用双缓冲区设计，生产者缓冲区和消费者缓冲区，如果生产者缓冲区中有内容就和消费者缓冲区交换，不是数据复制。

### 3.logSystm/logsCode/AsyncWorker.h
- 使用子线程异步处理日志，bool原子变量，互斥锁，条件变量的wait函数的使用（问题最大），子线程使用回调函数处理消费者缓冲区中的日志数据
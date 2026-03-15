#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"

class EventLoop;

class Channel : noncopyable
{
public:
    // C++类型别名定义，给复杂的函数类型起一个更简单的名字，方便后续使用
    // 通用函数包装器，可以保存任何没有参数、返回值为 void 的函数
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    
    void handleEvent(Timestamp receiveTime);

    void setReadCallback(ReadEventCallback cb) {}
};
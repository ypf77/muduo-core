#pragma once

#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "Timestamp.h"


// 前置声明的作用
// 告诉编译器：Channel 和 EventLoop 是类，但不需要现在知道它们的完整定义
// 编译器可以知道这个名字存在，可以用 指针或引用 声明变量或函数参数/返回值
// 避免不必要的头文件依赖，减少编译时间和循环依赖
class Channel;
class EventLoop;

class Poller
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *Channel) = 0;

    //判断参数channel是否在当前的Poller中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    // map的key:sockfd value:sockfd所属的channel通道类型
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    // 定义Poller所属的事件循环EventLoop    
    EventLoop *ownerLoop_;
};
#pragma once

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "Timestamp.h"

/**
 * epoll的使用:
epoll_create → 创建一个 事件收集器
epollfd → 收集器的“ID”，告诉内核往哪个收集器注册 socket
epoll_ctl → 往收集器里加/删 socket
epoll_wait → 查看收集器里哪些 socket “响铃”（有事件发生）
 **/

class Channel;

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel通道，其实就是调用epoll_ctl
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;      // epoll_create创建返回的fd保存在epollfd_中
    EventList events_; // 用于存放epoll_wait返回的所有发生的事件的文件描述符事件集
};
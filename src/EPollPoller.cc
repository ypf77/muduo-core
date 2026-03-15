#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

const int kNew = -1;    // 某个channel还没添加至Poller          // channel的成员index_初始化为-1
const int kAdded = 1;   // 某个channel已经添加至Poller
const int kDeleted = 2; // 某个channel已经从Poller删除

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        // LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    // 里面的 :: 是 C++ 的全局作用域运算符，也就是说，这里的 close 指的是全局命名空间下的函数，而不是类里可能有的 close 成员函数
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 由于频繁调用poll 实际上应该用LOG_DEBUG输出日志更为合理 当遇到并发场景 关闭DEBUG日志提升效率
    // LOG_INFO("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

    // epoll_wait 不会返回超过 maxevents 的事件，epoll_wait 本身不会丢事件，它保证 事件总会被返回，只是可能分多轮
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        // LOG_INFO("%d events happend\n", numEvents); // LOG_DEBUG最合理
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) // 扩容操作
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        // LOG_DEBUG("%s timeout!\n", __FUNCTION__);
    }
    else
    {
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            // LOG_ERROR("EPollPoller::poll() error!");
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    // LOG_INFO("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        else
        {
            // kDeleted 的含义：Channel 曾经注册到 epoll，但被删除（DEL）了
            // 现在调用 updateChannel 说明 它又有事件需要监听了
            // 所以要 重新添加回 epoll → 使用 EPOLL_CTL_ADD
            // 同时把 index 改回 kAdded，表示：现在它又在 epoll 中注册了
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    // LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    ::memset(&event, 0, sizeof(event));

    int fd = channel->fd();

    event.events = channel->events();
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            // LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            // LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}
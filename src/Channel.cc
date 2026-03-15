#include <sys/epoll.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

// C++ 类静态常量成员的定义和初始化
const int Channel::kNoneEvent = 0; //空事件
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI; //读事件
const int Channel::kWriteEvent = EPOLLOUT; //写事件

Channel::Channel(EventLoop *loop, int fd) 
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false)
{

}

Channel::~Channel()
{

}

// channel的tie方法什么时候调用过?  TcpConnection => channel
/**
解释：
TcpConnection 是 Channel 的“拥有者”
Channel 的回调函数都是 TcpConnection 的成员函数
如果 Channel 的事件触发时 TcpConnection 已经被销毁，就会悬空指针访问 → crash
 **/
void Channel::tie(const std::shared_ptr<void> &obj) 
{
    tie_ = obj;
    tied_ = true;
}

void Channel::update()
{
    // 通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    // loop_->updateChannel(this);
}

// 在channel所属的EventLoop中把当前的channel删除掉
void Channel::remove()
{
    // loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime) 
{
    if (tied_) 
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
        // 如果提升失败了 就不做任何处理 说明Channel的TcpConnection对象已经不存在了
    }
    else
    {
        // 直接调用回调，没有拥有者检查
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    // LOG_INFO("channel handleEvent revents:%d\n", revents_);

    // // 当TcpConnection对应Channel 通过shutdown 关闭写端 epoll触发EPOLLHUP
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }
    // 出现错误
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    // 读
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    // 写
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}


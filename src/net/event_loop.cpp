#include "event_loop.h"
#include "net_interface.h"

#include <cstdio>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include <map>

namespace netstack 
{
    extern std::map<uint32_t, NetInterface*> kNetifacesMap;

    RecvEventLoop::RecvEventLoop(ThreadPool& pool)
        : epollfd_(-1), start_(false), pool_(pool)
    {
        
    }

    RecvEventLoop::~RecvEventLoop()
    {
        start_ = false;
        recv_loop_thread_.Stop();
        
        
        if (epollfd_ != -1)
        {
            for (auto& netif : kNetifacesMap)
                epoll_ctl(epollfd_, EPOLL_CTL_DEL, netif.second->GetFd(), nullptr);
            close(epollfd_);
        }
    }
    
    bool RecvEventLoop::Start()
    {
        if (start_) 
            return true;

        if (InitEpoll() == false)
            return false;


        start_ = true;
        recv_loop_thread_.Start();
        recv_loop_thread_.SetThreadFunc(&RecvEventLoop::ThreadFunc, this);
        return true;
    }

    bool RecvEventLoop::InitEpoll()
    {
        epollfd_ = epoll_create1(0);
        if (epollfd_ == -1)
        {
            perror("epoll_create1 failed: ");
            return false;
        }

        struct epoll_event event;
        event.events = EPOLLIN; // 可读事件
        for (auto& netif : kNetifacesMap)
        {
            event.data.ptr = netif.second;
            if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, netif.second->GetFd(), &event) == -1)
            {
                perror("epoll_ctl ADD failed: ");
                return false;
            }
        }
        
        return true;
    }
    

    void RecvEventLoop::ThreadFunc()
    {
        std::vector<struct epoll_event> events(kNetifacesMap.size());
        while (start_)
        {
            int nfds = epoll_wait(epollfd_, events.data(), 10, -1);
            if (nfds == -1)
                continue;
            for (int i = 0; i < nfds; i++)
            {
                NetInterface* iface = reinterpret_cast<NetInterface*>(events[i].data.ptr);
                if (iface->NetRx() == false)     // 从网卡读取并存入到接收队列中,
                    continue;   // 读取失败因为队列满了,但是这种情况很小

                pool_.SubmitTask(HandleRecvPktCallback, iface);
            }
        }
    }

}
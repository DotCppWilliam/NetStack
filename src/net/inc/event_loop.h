/**
 * @file event_loop.h
 * @author william
 * @brief 用于接收网卡数据然后存放到网卡接口的接收队列中,然后通知线程池去处理
 * @version 0.1
 * @date 2024-09-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once


#include "noncopyable.h"
#include "sys_plat.h"
#include "threadpool.h"

#include <list>

namespace netstack
{
    class NetInterface;
    class RecvEventLoop : public NonCopyable
    {
    public:
        RecvEventLoop(std::list<NetInterface*>* netiflists, ThreadPool& pool);
        ~RecvEventLoop();
    public:
        bool Start();
    private:
        bool InitEpoll();
        void ThreadFunc();
    private:
        int epollfd_;
        bool start_;
        CustomThread recv_loop_thread_;
        std::list<NetInterface*>* netiflists_ = nullptr;
        ThreadPool& pool_;
    };

}
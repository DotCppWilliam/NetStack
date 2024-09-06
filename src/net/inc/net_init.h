#pragma once

#include "at_exit.h"
#include "event_loop.h"
#include "net_err.h"
#include "noncopyable.h"
#include "singlton.h"
#include "sys_plat.h"
#include "threadpool.h"
#include "timer.h"

namespace netstack 
{   
    struct NetInfo;
    class NetInit : public NonCopyable
    {
    public:
        NetInit();
        ~NetInit();
    public:
        NetErr_t Init(size_t threadpool_cnt = 0);
        ThreadPool& GetTaskThreadPool()
        { return pool; }
    public:
        static NetInit* GetInstance()
        {  return Singleton<NetInit>::get(); }
    private:
        bool initialized_ = false;
        PcapNICDriver driver_;              // 网卡驱动
        Timer* timer_ = nullptr;            // 定时器
        ThreadPool pool;                    // 线程池,用来处理接收、发送数据包的处理工作
        RecvEventLoop* event_loop_;         // 读取网卡数据包事件循环
        AtExitManager exit_manager_;
    };
}

#include "net_init.h"
#include "event_loop.h"
#include "loop.h"
#include "net_err.h"
#include "net_interface.h"
#include "net_pcap.h"
#include "sys_plat.h"
#include <vector>



#define TICK_TIME   ((50) * (1000000))  // 滴答时间
#define WHEEL_SIZE  (512)

namespace netstack 
{
    NetInit::NetInit()
    {

    }

    NetInit::~NetInit()
    {
        
    }

    NetErr_t NetInit::Init(size_t threadpool_cnt)
    {
        if (initialized_ == true)
            return NET_ERR_OK;

    // 初始化网卡接口列表
        NetInterface* netiface = nullptr;
        std::vector<NetInfo*>* devices = driver_.GetDevices();
        for (auto& device : *devices)
        {
            if (device->is_default_gateway_)
                netiface = new NetIfLoop(device);
            else
                netiface = new NetInterface(device);
            netifs_.push_back(netiface);
        }

    // 初始化线程池
        if (threadpool_cnt == 0)
            pool.Start();
        else 
            pool.Start(threadpool_cnt);

    // 初始化事件循环
        event_loop_ = new RecvEventLoop(&netifs_, pool);
        if (event_loop_ == nullptr)
            return NET_ERR_BAD_ALLOC;
        event_loop_->Start();

    // 初始化定时器 TODO:

        return NET_ERR_OK;
    }


    NetInfo* NetInit::GetNetworkInfo(uint32_t ip, std::string name)
    {
        return driver_.GetNetworkPtr(name, ip);
    }
}

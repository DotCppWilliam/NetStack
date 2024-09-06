#include "net_init.h"
#include "event_loop.h"
#include "loop.h"
#include "net_err.h"
#include "net_interface.h"
#include "net_pcap.h"
#include "sys_plat.h"
#include <vector>
#include <map>



#define TICK_TIME   ((50) * (1000000))  // 滴答时间
#define WHEEL_SIZE  (512)

namespace netstack 
{
    std::map<uint32_t, NetInterface*> kNetifacesMap;
    extern std::vector<NetInfo*> kDevices;

    NetInit::NetInit()
        : event_loop_(nullptr)
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
        for (auto& device : kDevices)
        {
            if (device->is_default_gateway_)
                netiface = new NetIfLoop(device);
            else
                netiface = new NetInterface(device);
            kNetifacesMap[*(uint32_t*)device->ip] = netiface;
        }

    // 初始化线程池
        if (threadpool_cnt == 0)
            pool.Start();
        else 
            pool.Start(threadpool_cnt);

    // 初始化事件循环
        event_loop_ = new RecvEventLoop(pool);
        if (event_loop_ == nullptr)
            return NET_ERR_BAD_ALLOC;
        event_loop_->Start();

    // 初始化定时器 TODO:

        return NET_ERR_OK;
    }
}

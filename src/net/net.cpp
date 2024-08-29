#include "net.h"
#include "exchange_msg.h"
#include "net_err.h"
#include "net_pcap.h"
#include "sys_plat.h"



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



    NetErr_t NetInit::Init()
    {
        if (initialized_)
            return NET_ERR_OK;

    // 初始化定时器 TODO: 暂时先不开启定时器
        // TimeEntry entry { { 0, TICK_TIME }};  // 滴答时间 0秒50毫秒
        // timer_ = new Timer(entry, WHEEL_SIZE);
        // timer_->Start();    // 启动定时器

    // 初始化网卡驱动
        driver_ = new PcapNICDriver();

    // 初始化网卡接口
        InitNetInterfaces(driver_->GetDevices());

    // 初始化消息队列
        exchange_msg_ = new ExchangeMsg();

    // 初始化收发线程
        msg_worker_ = new NetifPcap();

        initialized_ = true;
        return NET_ERR_OK;
    }
}

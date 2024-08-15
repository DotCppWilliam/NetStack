#include "ticker.h"
#include "time_entry.h"
#include "timer.h"

#include <bits/types/struct_timeval.h>
#include <cstdio>
#include <ctime>
#include <sys/select.h>
#include <unistd.h>


namespace netstack 
{
    Ticker::Ticker(TimeEntry tick_interval, Timer* timer)
        : tick_interval_(tick_interval), timer_obj_(timer) {}

    Ticker::~Ticker()
    {
        if (timer_obj_)
            timer_obj_ = nullptr;
    }

    void Ticker::Stop()
    {
        start_flag_ = false;   
    }

    /**
    * @brief 启动滴答
    * 
    */
    void Ticker::StartTick()
    {
        while (start_flag_)
        {
            // 获取滴答的时间
            timeval tick_interval = tick_interval_.GetTimeval();

            // 下面开始定时睡眠, 
            // TODO: 此处需要忽略一些信号,以免打乱定时睡眠
            nanosleep((const struct timespec*)&tick_interval, nullptr);

            // 滴答走完,将时间轮的指针向后走一格
            timer_obj_->AdvanceClock();
        }
    }

}
#pragma once

#include "time_entry.h"
#include "timer.h"

namespace netstack
{
    // 滴答,表示一个时钟走一格
    class Ticker
    {
    public:
        /**
        * @brief 构造函数
        * 
        * @param tick_interval 滴答的时间,也就是一个滴答是多长时间
        * @param timer 定时器
        */
        Ticker(TimeEntry tick_interval, Timer* timer);
        Ticker() = default;
        ~Ticker();
    public:
        void StartTick();   // 启动滴答
        void Stop();        // 停止
    private:
        TimeEntry tick_interval_;           // 滴答的时间
        Timer* timer_obj_;                  // 定时器
        volatile bool start_flag_ = true;   // 启动标志
    };
}
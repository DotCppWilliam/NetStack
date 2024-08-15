#pragma once


#include "time_entry.h"
#include "timer_task_list.h"
#include "delay_queue.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>


namespace netstack 
{
    // 时间轮
    class TimerWheel
    {
    public:
        TimerWheel(TimeEntry tick_ms, int wheel_size, TimeEntry start_ms,
            std::shared_ptr<std::atomic<int>> task_counter,
            std::shared_ptr<DelayQueue<TimerTaskList>> queue);
        ~TimerWheel();
    public:
        /**
        * @brief 
        * 
        * @param time_ms 
        */
        void AdvanceClock(TimeEntry time_ms);

        /**
        * @brief 
        * 
        * @param timer_task_entry 
        * @return true 
        * @return false 
        */
        bool Add(TimerTaskEntry* timer_task_entry);
    private:
        /**
        * @brief 
        * 
        */
        void AddOverflowWheel();
    private:
        TimeEntry tick_ms_;         // 每个时间槽(桶)之间的时间间隔,也就是时间轮的刻度
        TimeEntry start_ms_;        // 时间轮开始时的时间戳
        TimeEntry interval_;        // 时间轮转一圈的时间
        TimeEntry curr_time_ms_;    // 当前时间轮的时间,表示当前时间轮的起始时间

        std::mutex mutex_;
        int wheel_size_;            // 时间轮的轮数
        
        std::shared_ptr<std::atomic<int>> task_counter_;    // 任务计数器
        std::shared_ptr<DelayQueue<TimerTaskList>> queue_;  // 延迟队列,存放即将触发的任务列表
        std::vector<TimerTaskList> buckets_;                // 存放时间轮中的事件槽(桶),每个桶存储一定事件范围内的任务列表
        static TimerWheel* overflow_wheel_;    // 溢出时间的时间轮,当任务事件超过当前事件的范围时,任务会放到这个溢出时间轮中
    };
}
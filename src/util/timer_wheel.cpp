#include "timer_wheel.h"
#include "timer_task_list.h"
#include "time_entry.h"
#include <mutex>

namespace netstack 
{
    TimerWheel* TimerWheel::overflow_wheel_ = nullptr;

    TimerWheel::TimerWheel(TimeEntry tick_ms, int wheel_size, TimeEntry start_ms,
            std::shared_ptr<std::atomic<int>> task_counter,
            std::shared_ptr<DelayQueue<TimerTaskList>> queue)
        : tick_ms_(tick_ms), wheel_size_(wheel_size),
        task_counter_(task_counter), queue_(queue)
    {
        // 初始化用于时间轮的桶
        buckets_.resize(wheel_size, TimerTaskList(task_counter));

        // 时间轮转一圈的时间
        interval_ = tick_ms * wheel_size;

        // 
        curr_time_ms_ = start_ms - (start_ms % tick_ms);
    }

    TimerWheel::~TimerWheel()
    {
        if (overflow_wheel_)
            delete overflow_wheel_;
    }

    /**
    * @brief 推进时间轮的当前时间,如果给定时间已经超过当前轮的刻度,它会将时间轮的当前
    *       时间推进到新的时间.如果存放溢出时间轮,它也会相应的更新溢出时间轮的时间
    * 
    * @param time_ms 当前的时间戳
    */
    void TimerWheel::AdvanceClock(TimeEntry time_ms)
    {
        TimerWheel* curr_wheel = this;
        while (time_ms >= (curr_time_ms_ + tick_ms_))
        {
            curr_time_ms_ = time_ms - (time_ms % tick_ms_);
            if (overflow_wheel_ == nullptr)
                break;
        }
    }

    /**
    * @brief 添加一个新的任务到时间轮,如果任务到期时间在当前时间轮的范围内,
    *       则将任务添加到对应的时间槽中.如果超出当前时间轮的范围,
    *      则将任务添加到溢出时间轮中
    * 
    * @param timer_task_entry 
    * @return true 
    * @return false 
    */
    bool TimerWheel::Add(TimerTaskEntry* timer_task_entry)
    {
        TimeEntry expiration = timer_task_entry->GetExpiration();
        // 如果当前定时任务被取消或者该任务已经超时了,那么返回false
        if (timer_task_entry->Cancelled() || expiration < (curr_time_ms_ + tick_ms_))
        {
            printf("添加任务到时间轮失败\n");
            return false;
        }

        while (true)
        {
            if (expiration < (curr_time_ms_ + interval_))
            {
                long virtual_id = expiration / tick_ms_;
                TimerTaskList& bucket = buckets_.at(virtual_id % wheel_size_);
                bucket.Add(timer_task_entry);

                if (bucket.SetExpiration(tick_ms_ * virtual_id))
                    queue_->Offer(&bucket, expiration);

                return true;
            }
            else 
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (overflow_wheel_ == nullptr)
                {
                    overflow_wheel_ = new TimerWheel(
                        interval_,
                        wheel_size_,
                        curr_time_ms_,
                        task_counter_,
                        queue_
                    );
                }
            }
        }
        return true;
    }


    /**
    * @brief 创建并初始化一个新的溢出时间轮.当需要添加到溢出时间轮的任务时,
    *     如果溢出时间轮尚未存在,则调用此函数创建它
    * 
    */
    void TimerWheel::AddOverflowWheel()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (overflow_wheel_ == nullptr)
        {
            overflow_wheel_ = new TimerWheel(interval_, 
                wheel_size_, curr_time_ms_, 
                task_counter_, queue_);
        }
    }

}
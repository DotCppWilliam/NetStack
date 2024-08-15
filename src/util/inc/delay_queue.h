#pragma once

#include "def.h"
#include "time_entry.h"

#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>


namespace netstack
{
    // 自旋锁,如果要使用unique_lock,需要定义lock、unlock接口(必须的)
    class SpinLock 
    {
        using Ms = size_t;
    public:
        SpinLock(Ms acquire_failed_sleep_ms = Ms(-1));

        // 必须的接口
        void lock();
        void unlock();

        SpinLock& operator=(const SpinLock&) = delete;
        SpinLock(const SpinLock&) = delete;
    private:
        std::atomic<bool> locked_flag_ = ATOMIC_VAR_INIT(false);
        Ms acquire_failed_sleep_ms_;
    };




    // 延迟队列条目
    struct DelayQueueEntry
    {
        using Type = TimeEntry;

        DelayQueueEntry(void* append_carrier, Type expiration)
            : time_expiration_(expiration), append_carrier_(append_carrier) {}

        inline bool operator>(const DelayQueueEntry& delay_entry) const 
        {
            return time_expiration_.value_.tv_sec * TIME_BASE_NS 
                + time_expiration_.value_.tv_usec 
                > delay_entry.time_expiration_.value_.tv_sec * TIME_BASE_NS 
                + delay_entry.time_expiration_.value_.tv_usec;
        }

        Type time_expiration_;              // 延迟时间
        void* append_carrier_ = nullptr;    // 添加载体
    };



    // 延迟队列
    template <typename T>
    class DelayQueue
    {
    public:
        DelayQueue& operator=(const DelayQueue&) = delete;
        DelayQueue(const DelayQueue&) = delete;

        DelayQueue() = default;

        /**
        * @brief 获取一个定时任务到期的任务
        * 
        * @return T* 
        */
        T* Poll()
        {
            std::unique_lock<SpinLock> lock(spin_lock_);
            if (!priority_queue_.empty())   // 优先级队列非空
            {
                DelayQueueEntry first = priority_queue_.top();  // 获取队列头部元素
                if (GetDelay(first))    // 判断该延迟是否到期了
                    return nullptr;     // 如果未到期则返回nullptr
                else
                {
                    // 任务到期了
                    priority_queue_.pop(); // 从延迟队列中取出

                    // 返回该任务载体
                    return static_cast<T*>(first.append_carrier_);
                }
            }
            return nullptr;
        }

        /**
        * @brief 将定时任务和定时时间压入延迟队列中
        * 
        * @param time_carrier 定时任务
        * @param expiration 距离当前时间的延迟时间
        * @return true 
        * @return false 
        */
        bool Offer(T* time_carrier, TimeEntry expiration = TimeEntry( { -1, -1 }))
        {
            std::unique_lock<SpinLock> lock(spin_lock_);
            priority_queue_.push(DelayQueueEntry(time_carrier, expiration));
            return true;
        }

    private:
        /**
        * @brief 判断延迟时间是否到期了
        * 
        * @param entry 
        * @return true 延迟时间到期
        * @return false 未到期
        */
        bool GetDelay(const DelayQueueEntry& entry)
        {
            TimeEntry now;
            now.GetTimeofday(); // 获取当前时间戳

            // 延迟时间 - 当前时间, 如果还剩余时间的话,则返回true
        
            if ((entry.time_expiration_ - now) > TimeEntry({0, 0}))
                return true;

            return false;
        }
    private:
        SpinLock spin_lock_;    // 自旋锁
        std::priority_queue<DelayQueueEntry, std::vector<DelayQueueEntry>,
            std::greater<DelayQueueEntry>> priority_queue_;     // 优先级队列，指定底层存储结构为vector
    };
}
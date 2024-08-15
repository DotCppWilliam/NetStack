#pragma once

#include "delay_queue.h"
#include "time_entry.h"
#include "timer_task_list.h"
#include "threadpool.h"

#include <atomic>
#include <memory>
#include <shared_mutex>





namespace netstack 
{
    class Ticker;
    class ThreadPool;
    class TimerTaskList;
    class TimerWheel;
    class TimerTaskEntry;


    /**
    * @brief 定时器
    * 
    */
    class Timer
    {
    public:
        explicit Timer(TimeEntry tick_ms, int wheel_size, int ticker_num = 2,
            TimeEntry timeout = TimeEntry({ 0, 20000 }),
            int thread_num = 8);
        ~Timer();
    public:
        void AdvanceClock();
        int Size();
        void Shutdown();
        void Start();

        /**
        * @brief 添加定时任务
        * 
        * @param timer_task_entry 
        */
        void AddTimerTaskEntry(TimerTaskEntry* timer_task_entry);

        /**
        * @brief 添加延迟任务
        * 
        * @tparam Func 
        * @tparam Args 
        * @param delay_time 延迟时间
        * @param func 
        * @param args 
        */
        template <typename Func, typename... Args>
        void AddByDelay(TimeEntry delay_time, const Func&& func, Args&&... args)
        {
            auto fn = std::bind(func, std::forward<Args>(args)...);
            TimerTask* timer_task = new TimerTask(fn);
            TimeEntry now;
            now.GetTimeofday(); // 获取当前时间

            // 设置到期时间: 当前时间 + 延迟时间
            TimeEntry expiration = now + delay_time;
            // 添加任务到时间轮中
            Add(timer_task, expiration);
        }

        /**
        * @brief 添加
        * 
        * @tparam Func 
        * @tparam Args 
        * @param expiration 
        * @param func 
        * @param args 
        */
        template <typename Func, typename... Args>
        void AddByExpiration(TimeExpiration& expiration, const Func&& func, Args&&... args)
        {
            auto fn = std::bind(func, std::forward<Args>(args)...);
            TimerTask* timer_task = new TimerTask(fn);
            TimeEntry expir = expiration.GetTimeEntry();
            Add(timer_task, expir);
        }
    private:
        /**
        * @brief 
        * 
        * @param timer_task 定时任务
        * @param expiration 延迟时间
        */
        void Add(TimerTask* timer_task, TimeEntry expiration);
    private:
        std::unique_ptr<Ticker> ticker_;
        std::unique_ptr<ThreadPool> threadpool_;                    // 线程池
        std::shared_ptr<DelayQueue<TimerTaskList>> delay_queue_;    // 延迟队列
        std::unique_ptr<TimerWheel> timer_wheel_;                   // 时间轮
        std::shared_ptr<std::atomic<int>> task_counter_;            // 任务数量

        TimeEntry tick_ms_;             // 一次滴答所耗费的时间
        TimeEntry start_ms_;            // 开始定时的时间戳
        int wheel_size_;                // 时间轮大小
        int ticker_num_;                // 
        std::shared_mutex rw_mutex_;    // 读写锁,也叫共享锁
    };
}
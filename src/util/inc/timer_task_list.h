#pragma once


#include "time_entry.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>



namespace netstack 
{
    class TimerTaskEntry;
    class TimerTask;
    class TimerTaskList;
    class Timer;

    // 定时任务
    class TimerTask 
    {
    public:
        template <typename Func>
        explicit TimerTask(Func func)
            : timer_task_entry_(nullptr),
            func_(func) {}

        std::function<void()> GetFunc()
        { return func_; }

        void SetTimerTaskEntry(TimerTaskEntry* entry);
        TimerTaskEntry* GetTimerTaskEntry();

    private:
        TimerTaskEntry* timer_task_entry_;  // 定时任务条目
        std::function<void()> func_;
    };


    // 定时任务条目
    class TimerTaskEntry
    {
        friend TimerTaskList;
    public:
        TimerTaskEntry(TimerTask* timer_task, TimeEntry tv_expriation);
        TimerTaskEntry(const TimerTaskEntry& timer_task_entry);
        ~TimerTaskEntry()
        {
            if (prev_ != nullptr)
                prev_ = nullptr;

            if (next_ != nullptr)
                next_ = nullptr;

            if (timer_task_ != nullptr)
            {
                delete timer_task_;
                timer_task_ = nullptr;
            }
        }

        /**
        * @brief 
        * 
        * @return true 
        * @return false 
        */
        bool Cancelled();

        /**
        * @brief 当链表中删除当前条目
        * 
        */
        void Remove();

        /**
        * @brief 获取当前延迟时间
        * 
        * @return const TimeEntry 
        */
        const TimeEntry GetExpiration();

        /**
        * @brief 获取定时任务
        * 
        * @return TimerTask* 
        */
        TimerTask* GetTimerTask();
    public:
        TimerTaskList*  list_ = nullptr;    // 定时任务链表
        TimerTaskEntry* prev_ = nullptr;    // 指向上一个任务
        TimerTaskEntry* next_ = nullptr;    // 指向下一个任务

        TimerTask* timer_task_ = nullptr;   // 定时任务
        TimeEntry expiration_;              // 超时时间
    };



    // 定时任务链表
    class TimerTaskList
    {
    public:
        TimerTaskList(std::shared_ptr<std::atomic<int>> task_counter);
        TimerTaskList();
        TimerTaskList(const TimerTaskList& timer_task_list);
        TimerTaskList& operator=(const TimerTaskList& timer_task_list);
        ~TimerTaskList()
        {
            auto itd = root_->next_, itn = itd;
            while (itd->next_ != nullptr && itd != root_.get())
            {
                itn = itd->next_;
                delete itd;
                itd = itn;
            }
            
        }
    public:
        void Add(TimerTaskEntry* timer_task_entry);
        void Remove(TimerTaskEntry* timer_task_entry);

        /**
        * @brief 重置定时时间,并执行所有定时任务
        * 
        * @param callback_timer 
        */
        void Flush(Timer* callback_timer);
        bool SetExpiration(TimeEntry expiration);
        const TimeEntry GetExpiration();
    private:
        std::shared_ptr<std::recursive_mutex> mutex_;
        std::shared_ptr<TimerTaskEntry> root_;
        std::shared_ptr<std::atomic<int>> task_counter_;
        std::shared_ptr<std::atomic_flag> set_flag_;
        TimeEntry expiration_;
    };
}
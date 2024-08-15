#include "timer_task_list.h"
#include "time_entry.h"
#include "timer.h"

#include <atomic>
#include <memory>
#include <mutex>


namespace netstack 
{
    TimerTask* TimerTaskEntry::GetTimerTask()
    {
        return timer_task_;
    }

    void TimerTask::SetTimerTaskEntry(TimerTaskEntry* entry)
    {
        if (timer_task_entry_ != nullptr && timer_task_entry_ != entry)
            timer_task_entry_->Remove();

        timer_task_entry_ = entry;
    }

    /**
    * @brief 获取定时任务条目
    * 
    * @return TimerTaskEntry* 
    */
    TimerTaskEntry* TimerTask::GetTimerTaskEntry()
    {
        return timer_task_entry_;
    }





    ////////////////////////////////////////////////// TimerTaskEntry
    TimerTaskEntry::TimerTaskEntry(TimerTask* timer_task, TimeEntry tv_expiration)
        : expiration_(tv_expiration), timer_task_(timer_task)
    {
        if (timer_task != nullptr)
            timer_task->SetTimerTaskEntry(this);
    } 

    TimerTaskEntry::TimerTaskEntry(const TimerTaskEntry& timer_task_entry)
    {
        expiration_ = timer_task_entry.expiration_;
        timer_task_ = timer_task_entry.timer_task_;
        list_ = timer_task_entry.list_;
        prev_ = timer_task_entry.prev_;
        next_ = timer_task_entry.next_;
    }

    /**
    * @brief 
    * 
    */
    void TimerTaskEntry::Remove()
    {
        TimerTaskList* curr_list = list_;
        while (curr_list != nullptr)
        {
            // 从当前链表中删除this
            curr_list->Remove(this);
            curr_list = list_;
        }
    }

    bool TimerTaskEntry::Cancelled()
    {
        // 
        return timer_task_->GetTimerTaskEntry() != this;
    }

    /**
    * @brief 获取延迟时间
    * 
    * @return const TimeEntry 
    */
    const TimeEntry TimerTaskEntry::GetExpiration()
    {
        return expiration_;
    }








    ////////////////////////////////////////////////// TimerTaskList
    TimerTaskList::TimerTaskList() {}

    TimerTaskList::TimerTaskList(std::shared_ptr<std::atomic<int>> task_counter)
        : task_counter_(task_counter), mutex_(new std::recursive_mutex),
        set_flag_(new std::atomic_flag(ATOMIC_FLAG_INIT)),
        root_(new TimerTaskEntry(nullptr, TimeEntry()))
    {
        root_->next_ = root_.get();
        root_->prev_ = root_.get();
    }


    TimerTaskList::TimerTaskList(const TimerTaskList& timer_task_list)
    {
        mutex_ = timer_task_list.mutex_;
        root_ = timer_task_list.root_;
        root_->next_ = timer_task_list.root_->next_;
        root_->prev_ = timer_task_list.root_->prev_;
        task_counter_ = timer_task_list.task_counter_;
        set_flag_ = timer_task_list.set_flag_;
        expiration_ = timer_task_list.expiration_;
    }

    TimerTaskList& TimerTaskList::operator=(const TimerTaskList& timer_task_list)
    {
        if (&timer_task_list == this) 
            return *this;
        mutex_ = timer_task_list.mutex_;
        root_ = timer_task_list.root_;
        task_counter_ = timer_task_list.task_counter_;
        set_flag_ = timer_task_list.set_flag_;
        expiration_ = timer_task_list.expiration_;

        return *this;
    }

    void TimerTaskList::Add(TimerTaskEntry* timer_task_entry)
    {
        timer_task_entry->Remove();
        std::unique_lock<std::recursive_mutex> lock(*mutex_);
        if (timer_task_entry->list_ == nullptr)
        {
            TimerTaskEntry* tail = root_->prev_;
            timer_task_entry->next_ = root_.get();
            timer_task_entry->prev_ = tail;
            timer_task_entry->list_ = this;
            tail->next_ = timer_task_entry;
            root_->prev_ = timer_task_entry;
            (*task_counter_)++;
        }
    }

    /**
    * @brief 从当前链表中删除timer_task_entry
    * 
    * @param timer_task_entry 
    */
    void TimerTaskList::Remove(TimerTaskEntry* timer_task_entry)
    {
        // 递归锁
        std::unique_lock<std::recursive_mutex> lock(*mutex_);
        if (timer_task_entry->list_ == this)
        {
            timer_task_entry->next_->prev_ = timer_task_entry->prev_;
            timer_task_entry->prev_->next_ = timer_task_entry->next_;
            timer_task_entry->next_ = nullptr;
            timer_task_entry->prev_ = nullptr;
            timer_task_entry->list_ = nullptr;
            (*task_counter_)--;
        }
    }

    /**
    * @brief 重置定时时间,并执行所有定时任务
    * 
    * @param callback_timer 定时任务
    */
    void TimerTaskList::Flush(Timer* callback_timer)
    {
        std::unique_lock<std::recursive_mutex> lock(*mutex_);
        auto func = std::bind(&Timer::AddTimerTaskEntry, callback_timer, std::placeholders::_1);
        TimerTaskEntry* head = root_->next_;    // 获取链表中的头部定时条目

        // 移除所有的定时任务
        while (head != nullptr && head != root_.get())
        {
            Remove(head);
            func(head);
            head = root_->next_;
        }

        // 重置定时时间
        while (!set_flag_->test_and_set())
        {
            expiration_ = {-1, -1};
            set_flag_->clear();
            break;
        }
    }


    bool TimerTaskList::SetExpiration(TimeEntry expiration)
    {
        TimeEntry ori_expiration = expiration_;
        while (!set_flag_->test_and_set())
        {
            expiration_ = expiration;
            set_flag_->clear();
            return expiration != ori_expiration;
        }
        return expiration != ori_expiration;
    }


    const TimeEntry TimerTaskList::GetExpiration()
    {
        TimeEntry expiration = expiration_;
        while (!set_flag_->test_and_set())
        {
            set_flag_->clear();
            return expiration;
        }
        return expiration;
    }

}



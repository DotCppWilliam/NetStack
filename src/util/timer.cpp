#include "timer.h"
#include "delay_queue.h"
#include "threadpool.h"
#include "time_entry.h"
#include "timer_task_list.h"
#include "ticker.h"
#include "timer_wheel.h"
#include <memory>
#include <mutex>
#include <shared_mutex>


namespace netstack 
{
    Timer::Timer(TimeEntry tick_ms, int wheel_size, int ticker_num,
        TimeEntry timeout, int thread_num)
        : tick_ms_(tick_ms), wheel_size_(wheel_size),
        ticker_num_(ticker_num), task_counter_(new std::atomic<int>(0)),
        ticker_(new Ticker(timeout, this)),
        delay_queue_(new DelayQueue<TimerTaskList>()),
        threadpool_(new ThreadPool)
    {
        start_ms_.GetTimeofday();   // 记录开始定时的时间

        // 创建一个时间轮
        // 设置滴答的时间、时间轮中轮的个数、定时起始时间、任务计时器、延迟队列
        timer_wheel_ = std::make_unique<TimerWheel>(tick_ms_, wheel_size_, 
            start_ms_, task_counter_, delay_queue_);
        threadpool_->Start();   // 启动线程池
    }

    void Timer::Start()
    {
        for (int i = 0; i < ticker_num_; i++)
        {   
            threadpool_->SubmitTask(&Ticker::StartTick, ticker_.get());
        }
    }

    /**
    * @brief 
    * 
    * @param timer_task 定时任务
    * @param expiration 到期时间
    */
    void Timer::Add(TimerTask* timer_task, TimeEntry expiration)
    {
        // 创建读写锁
        std::shared_lock<std::shared_mutex> lock(rw_mutex_);
        // 向时间轮添加定时任务
        AddTimerTaskEntry(new TimerTaskEntry(timer_task, expiration));
    }

    /**
    * @brief 添加定时任务到时间轮中
    * 
    * @param timer_task_entry 
    */
    void Timer::AddTimerTaskEntry(TimerTaskEntry* timer_task_entry)
    {
        // 向时间轮添加定时任务如果失败的话
        if (!timer_wheel_->Add(timer_task_entry))   
        {
            if (!timer_task_entry->Cancelled()) // 则取消定时任务
            {
                // 如果取消定时任务失败的话

                // 则下面将任务提交到线程池中
                TimerTask* timer_task = timer_task_entry->GetTimerTask();
                auto fn = timer_task->GetFunc();
                fn();
                threadpool_->SubmitTask(timer_task->GetFunc());
                delete timer_task_entry;
            }
        }
    }


    void Timer::AdvanceClock()
    {
        auto bucket = delay_queue_->Poll();
        if (bucket != nullptr)
        {
            std::unique_lock<std::shared_mutex> lock(rw_mutex_);
            while (bucket != nullptr)
            {
                timer_wheel_->AdvanceClock(bucket->GetExpiration());
                bucket->Flush(this);
                bucket = delay_queue_->Poll();
            }
        }
    }


    void Timer::Shutdown()
    {
        ticker_->Stop();
        threadpool_->Exit();
    }


    Timer::~Timer()
    {
        ticker_.reset();
        threadpool_.reset();
        delay_queue_.reset();
        timer_wheel_.reset();
    }

}
#include "threadpool.h"
#include <memory>
#include <spdlog/spdlog.h>


namespace netstack 
{
    //////////////////////////////////////////////////////// Thread
    int Thread::generate_id_ = 0;
    Thread::Thread(Func func)
        : func_(func),
        thread_id_(generate_id_++) {}


    void Thread::Start()
    {
        std::thread t(func_, thread_id_);
        t.detach();
    }


    int Thread::GetId() const
    {
        return thread_id_;
    }



    //////////////////////////////////////////////////////// ThreadPool
    ThreadPool::ThreadPool()
        : init_thread_cnt_(0),
        mode_(PoolMode::MODE_CACHED),   // 默认为动态模式
        running_(false), task_queue_(std::thread::hardware_concurrency() + 20)
    {
    }


    ThreadPool::~ThreadPool()
    { 
        Exit(); 
    
    }


    void ThreadPool::Exit()
    {
        running_ = false;
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_cond_.notify_all();
        exit_cond_.wait(lock, [&]()->bool { return threads_.empty(); });
    }


    void ThreadPool::SetMode(PoolMode mode)
    {
        if (running_)
            return;
        mode_ = mode;
    }


    void ThreadPool::SetThreadMaxThreshold(int threshold)
    {
        if (running_)
            return;
        if (mode_ == PoolMode::MODE_CACHED)
            thread_max_threshold_ = threshold;
    }


    void ThreadPool::SetTaskMaxThreshold(int threshold)
    {
        if (running_)
            return;
        task_max_threadshold_ = threshold;
    }


    void ThreadPool::Start(int init_thread_cnt)
    {
        running_ = true;
        init_thread_cnt_ = curr_thread_cnt_ = init_thread_cnt;
        for (int i = 0; i < init_thread_cnt_; i++)
        {
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this, 
                std::placeholders::_1));
            int threadId = ptr->GetId();
            threads_.emplace(threadId, std::move(ptr));
        }
        for (int i = 0; i < init_thread_cnt_; i++)
        {
            threads_[i]->Start();
            idle_thread_cnt_++;
        }
    }
    /**
    * @brief 线程执行函数
    * 
    * @param thread_id 
    */
    void ThreadPool::ThreadFunc(int thread_id)
    {   
        auto last_time = std::chrono::high_resolution_clock().now();
        Task task;
        while (running_)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);

                // 队列为空
                if (task_queue_.Empty())
                {
                    if (mode_ == PoolMode::MODE_CACHED)
                    {
                        if (std::cv_status::timeout == 
                            not_empty_cond_.wait_for(lock, std::chrono::seconds(1)))
                        {
                            auto now = std::chrono::high_resolution_clock().now();
                            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_time);
                            if (duration.count() >= kThreadMaxIdleTime
                                && curr_thread_cnt_ > init_thread_cnt_)
                            {
                                threads_.erase(thread_id);
                                curr_thread_cnt_--;
                                idle_thread_cnt_--;
                                return;
                            }
                        }
                    }
                    else
                        not_empty_cond_.wait(lock);
                }
            
                idle_thread_cnt_--;
                bool ret = task_queue_.TryPop(task);
                if (ret)
                    task_cnt_--;
                if (!task_queue_.Empty())
                    not_empty_cond_.notify_all();
                not_full_cond_.notify_all();
            }
            if (task)
            {
                task();
                task = nullptr;
            }
            idle_thread_cnt_++;
            last_time = std::chrono::high_resolution_clock().now();
        }
        threads_.erase(thread_id);
        spdlog::info("thread_id: {} exit", thread_id);
        exit_cond_.notify_all();
    }

}
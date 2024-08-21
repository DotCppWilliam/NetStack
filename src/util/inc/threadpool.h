#pragma once

#include "concurrent_queue.h"
#include "noncopyable.h"


#include <chrono>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>

/*
    线程数量如何确定:
        CPU 密集型任务: 建议线程数量与CPU核心数量相等或略多.因为每个线程需要CPU时间片,
            如果线程数量过多,可能会引发频繁的上下文切换,反而降低性能
        IO密集型任务: 比如网络请求、文件读写等.线程数可以增加到CPU核心数量的两倍甚至更多,
            这是因为IO操作通常会阻塞线程,增加线程可以提高系统的并发能力
    
    如果任务执行时间较短,那么设置较少的线程数
    如果执行任务时间长,且任务之间没有太多依赖,可以增加线程数提高并行处理能力.
    经验法则:
        线程数 = CPU核心数 + 1(适用于密集型) 或者
        线程数 = CPU核心数 * 2(IO密集型)
*/

const int kTaskMaxThreshold = 512;
const int kThreadMaxThreshold = 1024;
const int kThreadMaxIdleTime = 60;


namespace netstack 
{
    enum class PoolMode
    {
        MODE_FIXED,     // 固定线程数量模式
        MODE_CACHED     // 动态线程数量模式
    };
    // 线程
    class Thread
    {
        using Func = std::function<void(int)>;
    public:
        Thread(Func func);
        ~Thread() {}
        void Start();
        int GetId() const;
    private:
        Func func_;
        static int generate_id_;
        int thread_id_;
    };
    // 线程池
    class ThreadPool : private NonCopyable
    {
        using Task = std::function<void()>;
    public:
        ThreadPool();
        ~ThreadPool();
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        void Exit();
        /**
        * @brief 设置线程池工作模式: 固定线程数量、动态线程数量
        * 
        * @param mode 
        */
        void SetMode(PoolMode mode);
        /**
        * @brief 设置线程数量最大上限. 默认1024
        * 
        * @param threshold 
        */
        void SetThreadMaxThreshold(int threshold = kThreadMaxThreshold);
        /**
        * @brief 设置任务最大上限. 默认512
        * 
        * @param threshold 
        */
        void SetTaskMaxThreshold(int threshold = kTaskMaxThreshold);
        template <typename Func, typename... Args>
        auto SubmitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>
        {
            if (running_ == false)
                std::runtime_error("Threadpool is no running!!!");

            using RType = decltype(func(args...));	
            auto task = std::make_shared<std::packaged_task<RType()>>(
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
            std::future<RType> result = task->get_future();
            int ret = 0;
            _SubmitTask<RType>(task, ret);
            if (ret != 0)
            {
                auto retNull = std::make_shared<std::packaged_task<RType()>>(
                    []()->RType { return RType(); });
                (*retNull)();
                return retNull->get_future();
            }
            return result;
        }
        /**
        * @brief 提交任务.提交成员函数
        * 
        * @tparam Func 成员函数签名
        * @tparam Obj 对象类型
        * @tparam Args 函数参数类型
        * @param func 成员函数地址
        * @param obj 对象地址
        * @param args 函数用到的参数
        * @return std::future<typename std::result_of<Func(Obj, Args...)>::type> 
        */
        template <typename Func, typename Obj, typename... Args>
        std::future<typename std::result_of<Func(Obj, Args...)>::type>
            SubmitTask(Func&& func, Obj&& obj, Args&&... args)
        {
            if (running_ == false)
                std::runtime_error("Threadpool is no running!!!");

            using RType = typename std::result_of<Func(Obj, Args...)>::type;
            auto task = std::make_shared<std::packaged_task<RType()>>(
                std::bind(std::forward<Func>(func), std::forward<Obj>(obj), 
                    std::forward<Args>(args)...)
            );
            std::future<RType> result = task->get_future();
            int ret = 0;
            _SubmitTask<RType>(task, ret);
            if (ret != 0)
            {
                auto ret_null = std::make_shared<std::packaged_task<RType()>>(
                    []()->RType { return RType(); }
                );
                (*ret_null)();
                return ret_null->get_future();
            }
            return result;
        }
        /**
        * @brief 启动线程池
        * 
        * @param initThreadCnt 
        */
        void Start(int initThreadCnt = std::thread::hardware_concurrency());
    private:    
        void ThreadFunc(int thread_id); // 线程执行函数
        template <typename RType>
        void _SubmitTask(std::shared_ptr<std::packaged_task<RType()>> task, int& ret)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            auto task_lambda = [task]() { (*task)(); };
            if (!task_queue_.TryEmplace(task_lambda))
            {
                // 如果插入失败,那么等待10ms在插入
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (!task_queue_.TryEmplace(task_lambda))
                {
                    ret = 1;    // 插入失败则返回
                    return;
                }
            }
            not_empty_cond_.notify_all();   // 通知所有线程有任务到来
            if (mode_ == PoolMode::MODE_CACHED
                && task_cnt_ > (idle_thread_cnt_ * 2) /* 任务数量大于空闲数量的两倍 */
                && curr_thread_cnt_ < thread_max_threshold_)
            {
                // 创建更多线程
                int create_task_cnt = (task_cnt_ - idle_thread_cnt_) / 2;
                for (int i = 0; i < create_task_cnt; i++)
                {
                    auto ptr = new Thread(std::bind(&ThreadPool::ThreadFunc, this, 
                        std::placeholders::_1));
                    int thread_id = ptr->GetId();
                    threads_.emplace(thread_id, std::move(ptr));
                    threads_[thread_id]->Start();
                }
                
                curr_thread_cnt_ += create_task_cnt;
                idle_thread_cnt_ += create_task_cnt;
            }
            
        }
    private:
        std::unordered_map<int, std::unique_ptr<Thread>> threads_;  // 线程列表
        int init_thread_cnt_;                               // 线程初始数量
        int thread_max_threshold_ = kThreadMaxThreshold;    // 线程最大数量上限
        std::atomic<int> curr_thread_cnt_ = 0;              // 当前线程数量
        std::atomic<int> idle_thread_cnt_ = 0;              // 空闲线程数量
        PoolMode mode_;                                     // 线程池工作模式
        std::atomic<bool> running_ = false;                 // 线程池工作状态
        std::atomic<int> task_cnt_ = 0;
        int task_max_threadshold_ = kTaskMaxThreshold;
        ConcurrentQueue<Task> task_queue_;
        std::mutex mutex_;
        std::condition_variable not_full_cond_;     // 队列非满条件变量
        std::condition_variable not_empty_cond_;    // 队列非空条件变量
        std::condition_variable exit_cond_;         // 线程池退出条件变量
    };
}
#pragma once

#include "net_err.h"
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <sys/time.h>
#include <sys/sem.h>
#include <pthread.h>
#include <pcap.h>
#include <thread>
#include <chrono>
#include <type_traits>
#include <vector>


using net_time_t = struct timeval;

#define SYS_THREAD_INVALID  (sys_thread_t)0
#define SYS_SEM_INVALID     (sys_sem_t)0
#define SYS_MUTEX_INVALID   (sys_mutex_t)0

#define plat_strlen         strlen
#define plat_strcpy         strcpy
#define plat_strncpy        strncpy
#define plat_strcmp         strcmp
#define plat_stricmp        strcasecmp
#define plat_memset         memset
#define plat_memcpy         memcpy
#define plat_memcmp         memcmp
#define plat_sprintf        sprintf
#define plat_vsprintf       vsprintf
#define plat_printf         printf
#define plat_sleep(ms)      std::this_thread::sleep_for(std::chrono::seconds(ms));



// 网卡类型
enum NetIfType {
    NETIF_TYPE_NONE = 0,
    NETIF_TYPE_ETHER,       // 普通网卡
    NETIF_TYPE_LOOP,        // 回环网卡
};


namespace lpcap 
{
    typedef struct x_sys_sem_t {
        int count_;                 // 信号量计数
        pthread_cond_t cond_;       // 条件变量
        pthread_mutex_t locker_;    // 互斥锁
    } *sys_sem_t;

    using SysThread = pthread_t;
    using SysMutex = pthread_mutex_t*;


    // pcap网卡驱动
    class PcapNICDriver
    {
    public:
        PcapNICDriver(const char* ip) : ip_(ip) {}
        PcapNICDriver() {}
        ~PcapNICDriver();

        bool FindDevice(char* name_buf);
        bool ShowList();
        net::NetErr_t DeviceOpen(const uint8_t* mac_addr);
        bool OpenAllDefaultDevice();
        bool IsOpened() const 
        {
            return devices_.empty();
        }
    private:
        const char* ip_;
        std::vector<pcap_t*> devices_;
    };


    // 信号量
    class SysSemaphore
    {
    public:
        SysSemaphore(int init_count)
        {
            if (!Create(init_count))
                throw "create semaphore failed";
        }

        ~SysSemaphore() { Free(); }

        bool Create(int init_count);
        void Free();

        bool Wait(uint32_t ms);
        void Notify();
    private:
        sys_sem_t sem_ = nullptr;
    };


    // 线程
    class Thread
    {
    public:
        Thread() {}
        ~Thread() {}

        void Start();
        void Stop();

        template <typename Func, typename... Args>
        auto SetThreadFunc(Func&& func, Args&&... args) -> 
            std::future<decltype(func(args...))>
        {
            using RType = decltype(func(args...));
            while (!is_running_)
                continue;

            auto task = std::make_shared<std::packaged_task<RType()>>(
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
            );
            std::future<RType> result = task->get_future();
            {
                std::unique_lock<std::mutex> lock(mutex_);
                func_ = [task]{
                    (*task)();
                };
            }
            cond_running_.notify_all();
            return result;
        }

        template <typename Func, typename Obj, typename... Args>
        std::future<typename std::result_of<Func(Obj, Args...)>::type>
            SetThreadFunc(Func&& func, Obj&& obj, Args&&... args)
        {
            using RType = typename std::result_of<Func(Obj, Args...)>::type;
            while (!is_running_)
                continue;

            auto task = std::make_shared<std::packaged_task<RType()>>(
                std::bind(std::forward<Func>(func), std::forward<Obj>(obj),
                    std::forward<Args>(args)...)
            );
            std::future<RType> result = task->get_future();
            {
                std::unique_lock<std::mutex> lock(mutex_);
                func_ = [task]{
                    (*task)();
                };
            }
            cond_running_.notify_all();
            return result;
        }
    private:
        void ThreadFunc();
    private:
        std::function<void()> func_ = nullptr;
        std::thread thread_;
        bool is_running_ = false;
        std::condition_variable cond_running_;  // 通知线程有任务到来
        bool exit_ = false;
        std::mutex mutex_;
    };
}


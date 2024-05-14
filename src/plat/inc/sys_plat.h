#pragma once

#include <sys/time.h>
#include <sys/sem.h>
#include <pthread.h>
#include <pcap.h>
#include <thread>
#include <chrono>


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

namespace lpcap 
{
    typedef struct x_sys_sem_t {
        int count_;                 // 信号量计数
        pthread_cond_t cond_;       // 条件变量
        pthread_mutex_t locker_;    // 互斥锁
    } *sys_sem_t;

    using sys_thread_t = pthread_t;
    using sys_mutex_t = pthread_mutex_t*;


    // pcap网卡驱动
    class PcapNICDriver
    {
    public:
        PcapNICDriver(const char* ip) : ip_(ip) {}
        ~PcapNICDriver() {}

        bool FindDevice(char* name_buf);
        bool ShowList();
        pcap_t* DeviceOpen(const uint8_t* mac_addr);
    private:
        const char* ip_;
    };


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

    class SysThread 
    {
    public:
        using SysThreadFunc_t = void(*)(void*);

        SysThread(SysThreadFunc_t entry, void* arg)
            : func_(entry), arg_(arg) {}
        
        bool Create();
        void Sleep(int ms);
        sys_thread_t Self();
    private:
        sys_thread_t thread_;
        SysThreadFunc_t func_;
        void* arg_;
    };
}


#include "delay_queue.h"
#include <chrono>
#include <thread>

namespace netstack 
{
    SpinLock::SpinLock(Ms acquire_failed_sleep_ms)
        : acquire_failed_sleep_ms_(acquire_failed_sleep_ms) {}

    void SpinLock::unlock()
    { locked_flag_.store(false); }

    void SpinLock::lock()
    {
        bool exp = false;
        while (!locked_flag_.compare_exchange_strong(exp, true))
        {
            exp = false;
            if (acquire_failed_sleep_ms_ == Ms(-1))
                std::this_thread::yield();
            else if (acquire_failed_sleep_ms_ != 0)
                std::this_thread::sleep_for(std::chrono::microseconds(acquire_failed_sleep_ms_));
        }
    }
}
#pragma once

#include <mutex>

namespace util
{
    class Mutex 
    {
    public:
        Mutex() { }
        ~Mutex() {}

    protected:
        void Lock()
        { mutex_.lock();}

        void UnLock()
        { mutex_.unlock(); }

        bool TryLock()
        { return mutex_.try_lock(); }
    private:
        std::mutex mutex_;
    };

    class Lock : public Mutex 
    {
    public:
        Lock() {}
        ~Lock() {}

        void Acquire() { Lock(); }
        void Release() { UnLock(); }// 释放锁
        bool Try() { return TryLock(); }
        void AssertAcquired() const {}
    };

    class AutoLock
    {
    public:
        struct AlreadyAcquired {};
        explicit AutoLock(Lock& lock)
            : lock_(lock) 
        {
            lock_.Acquire();
        }

        AutoLock(Lock& lock, const AlreadyAcquired&)
            : lock_(lock)
        {
            lock.Acquire();
        }

        ~AutoLock()
        {
            lock_.Release();
        }
    private:
        Lock& lock_;
    };


    class AutoUnLock
    {
    public:
        explicit AutoUnLock(Lock& lock)
            : lock_(lock) 
        {
            lock_.Release();
        }


        ~AutoUnLock()
        {
            lock_.Acquire();
        }
    private:
        Lock& lock_;
    };
}
#pragma once

#include "sys_plat.h"
#include "auto_lock.h"

#include <list>

namespace net 
{
    class MemBlock 
    {
    public:

    private:
        lpcap::SysSemaphore alloc_sem_;
        util::AutoLock lock_; 
        // 链表
        
    };
}
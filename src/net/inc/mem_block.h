#pragma once

#include "sys_plat.h"
#include "auto_lock.h"

#include <list>

namespace netstack 
{
    class MemBlock 
    {
    public:

    private:
        SysSemaphore alloc_sem_;
        AutoLock lock_; 
        // 链表
        
    };
}
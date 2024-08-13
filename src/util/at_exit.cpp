#include "at_exit.h"
#include "auto_lock.h"

#include <cassert>

namespace netstack 
{
    static AtExitManager* g_top_manager = nullptr;

    AtExitManager::AtExitManager()
        : next_manager_(g_top_manager)
    {
        g_top_manager = this;
    }

    AtExitManager::~AtExitManager()
    {
        assert(!g_top_manager);
        ProcessCallBackNow();
        g_top_manager = next_manager_;
    }

    void AtExitManager::RegisterCallBack(AtExitCallBackType func, void* parm)
    {
        assert(!g_top_manager);
        AutoLock lock(g_top_manager->lock_);
        g_top_manager->stack_.push( { func, parm });
    }

    void AtExitManager::ProcessCallBackNow()
    {
        assert(!g_top_manager);

        AutoLock lock(g_top_manager->lock_);
        while (!g_top_manager->stack_.empty())
        {
            // 获取一个任务
            CallBack task = g_top_manager->stack_.top();
            task.func(task.parm);
            g_top_manager->stack_.top();
        }
    }

    
}
#pragma once

#include <stack>

#include "auto_lock.h"


namespace netstack 
{
    class AtExitManager
    {
        using AtExitCallBackType = void(*)(void*);
    public:
        AtExitManager();
        ~AtExitManager();

        static void RegisterCallBack(AtExitCallBackType func, void* parm);
        static void ProcessCallBackNow();
    protected:
        explicit AtExitManager(bool shadow);
    private:
        struct CallBack 
        {
            AtExitCallBackType func;
            void* parm;
        };

        std::stack<CallBack> stack_;
        AtExitManager* next_manager_;
        Lock lock_;
    };
}
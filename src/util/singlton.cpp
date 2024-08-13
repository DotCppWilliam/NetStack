#include "singlton.h"

namespace netstack 
{
    /**
     * @brief 
     * 
     * @param instance 
     * @return AtomicWord 
     */
    AtomicWord WaitForInstance(AtomicWord *instance)
    {
        AtomicWord value;

        while (true)
        {
            // 原子获取对象实例
            value = AcquireLoad(instance);

            // 如果此时value不等于kBeingCreatedMarker,表示对象实例创建完,可以返回
            if (value != kBeingCreatedMarker)
                break;

            // value等于kBeingCreatedMarker表示正在创建对象实例,等待
            sched_yield();  // 让出处理器
        }
        return value;
    }
    
}
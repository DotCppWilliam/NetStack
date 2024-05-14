#pragma once

#include <cstdint>
namespace util 
{
    #ifdef __x86_64
        using AtomicWord = uint64_t;
    #elif 
        using AtomicWord = uint32_t;
    #endif 

    
    /**
     * @brief 原子操作,将new_value设置到ptr,并返回设置之前的ptr值
     * 
     * @param ptr 要设置的内存
     * @param new_value 设置到ptr的值
     * @return AtomicWord 
     */
    inline AtomicWord NoBarrierCompareAndSwap(volatile AtomicWord* ptr,
        AtomicWord new_value)
    {
        AtomicWord old_val;
        do {
            old_val = *ptr; // 保存CAS之前ptr的值

            // 比较ptr和old_val,如果相同则将new_value更新到ptr并返回true
        } while (!__sync_bool_compare_and_swap(ptr, old_val, new_value));
        return old_val;
    }



    /**
     * @brief 如果ptr和old_val相等并将new_val设置到ptr,并返回old_val. 否则ptr值,并不设置值
     * 
     * @param ptr 
     * @param old_val 
     * @param new_val 设置的值
     * @return AtomicWord 
     */
    inline AtomicWord NoBarrierCompareAndSwap(volatile AtomicWord* ptr,
                                            AtomicWord old_val,
                                            AtomicWord new_val)
    {
        AtomicWord prev_val;
        do {
            if (__sync_bool_compare_and_swap(ptr, old_val, new_val))
                return old_val;
            prev_val = *ptr;
        } while (prev_val == old_val);
        return prev_val;
    }

    /**
     * @brief 原子操作获得值,保证代码中再次之前的写操作都完成
     * 
     * @param ptr 
     * @return AtomicWord 
     */
    inline AtomicWord AcquireLoad(volatile const AtomicWord* ptr)
    {
        AtomicWord val = *ptr;
        __sync_synchronize();
        return val;
    }

    /**
     * @brief 原子写操作,和原子获取值差不多,也都是保证在代码中再次之前的写操作一定都完成
     * 
     * @param ptr 
     * @param val 
     */
    inline void ReleaseStore(volatile AtomicWord* ptr, AtomicWord val)
    {
        __sync_synchronize();
        *ptr = val;
    }



    /**
     * @brief 原子比较交换
     * 
     * @param ptr 
     * @param old_val 
     * @param new_val 
     * @return AtomicWord 
     */
    inline AtomicWord AcquireCompareAndSwap(volatile AtomicWord* ptr,
                                            AtomicWord old_val,
                                            AtomicWord new_val)
    {
        return NoBarrierCompareAndSwap(ptr, old_val, new_val);
    }

    
}
#pragma once

#include "atomic_cmps.h"
#include "at_exit.h"

namespace util
{
    static const AtomicWord  kBeingCreatedMarker = 1;

    AtomicWord WaitForInstance(AtomicWord* instance);

    
///////////////////////////////////////////////////////// DefaultSingletonTraits    
    /**
     * @brief Singleton默认使用的Traits.用于Singleton分配对象、析构对象
     *          在堆上分配内存和释放内存
     * 
     * @tparam Type 
     */
    template <typename Type>
    struct DefaultSingletonTraits
    {
        /**
         * @brief 分配对象
         * 
         * @return Type* 
         */
        static Type* New()
        {
            return new Type();
        }

        /**
         * @brief 析构对象
         * 
         * @param x 
         */
        static void Delete(Type* x)
        {
            delete x; 
        }

        // 如果为true在进程退出时自动将对象删除. [默认true]
        static const bool kRegisterAtExit;

        // 为false禁止访问不可joinable线程. [默认false]
        static const bool kAllowedToAcessOnNonJoinableThread;
    };
// 静态变量初始化
    template <typename Type>
    const bool DefaultSingletonTraits<Type>::kRegisterAtExit = true;

    template <typename Type>
    const bool DefaultSingletonTraits<Type>::kAllowedToAcessOnNonJoinableThread = false;





///////////////////////////////////////////////////////// AlignedMemory
    /**
     * @brief 
     * 
     * @tparam Size 
     * @tparam ByteAlignment 
     */
    template <size_t Size, size_t ByteAlignment>
    class AlignedMemory
    {
    public:
        uint8_t data_[Size];
        void* void_data() 
        {
            return static_cast<void*>(data_);
        }

        template <typename Type>
        Type* data_as() { return static_cast<Type*>(void_data()); }

        template <typename Type>
        const Type* data_as() const 
        {
            return static_cast<const Type*>(void_data());
        }
    private:
        void* operator new(size_t);
        void operator delete(void*);
    };




///////////////////////////////////////////////////////// StaticMemorySingletonTraits
    /**
     * @brief 在内部缓冲区构造和析构对象
     * 
     * @tparam Type 
     */
    template <typename Type>
    struct StaticMemorySingletonTraits
    {
        static Type* New()
        {
            // CAS, 将dead_设置为1,并返回dead_之前的值
            if (NoBarrierCompareAndSwap(&dead_, 1))
                return nullptr; // 如果dead_之前就为1表示已经设置过则返回nullptr
            
            // 重定位new,在buffer_上构造对象
            return new(buffer_.void_data()) Type();
        }

        static void Delete(Type* p)
        {
            if (p != nullptr)
                p->Type::~Type();   // 析构对象
        }
    private:
        static AtomicWord dead_;
        static AlignedMemory<sizeof(Type), alignof(Type)> buffer_;
    };
// 静态变量初始化
    template <typename Type>
    AtomicWord StaticMemorySingletonTraits<Type>::dead_ = 0;

    template <typename Type>
    AlignedMemory<sizeof(Type), alignof(Type)> StaticMemorySingletonTraits<Type>::buffer_;



///////////////////////////////////////////////////////// Singleton

    /**
     * @brief 单例模式类
     *              1. 该类是线程安全的
     *              2. 该类没有非静态成员,不需要实例化
     *              3. 使用该类必须自己声明一个 Type* GetInstance()函数
     *              4. 可以安全的构造和复制构造,因为没有成员
     * @tparam Type 
     * @tparam Traits 
     * @tparam DifferentiatingType 
     */
    template <typename Type, 
            typename Traits = DefaultSingletonTraits<Type>,
            typename DifferentiatingType = Type>
    class Singleton
    {
        // 使用模板类应该声明一个GetInstance()方法,其中调用Singleton::get()
        friend Type* Type::GetInstance();

        static Type* get()
        {
            // 将instance_值存储到value,写入操作是原子操作
            AtomicWord value = AcquireLoad(&instance_);
            // 判断是否创建完对象实例
            if (value != 0 && value != kBeingCreatedMarker)
                return reinterpret_cast<Type*>(value);  // 成功获取到,并且对象实例已经创建
            
            // 还没有创建实例
            if (0 == AcquireCompareAndSwap(&instance_, 0, kBeingCreatedMarker))
            {
                // 进入到该语句块里instance_值为1,表示准备创建实例,但是还没有进行完,其他线程看见会等待

                Type* new_value = Traits::New();    // 创建对象
                // 将new_value写入instance_成员变量中,该操作是原子的不会被中断
                ReleaseStore(&instance_, reinterpret_cast<AtomicWord>(new_value));


                // 设置了kRegisterAtExit则进程退出自动销毁
                if (new_value != nullptr && Traits::kRegisterAtExit)
                    AtExitManager::RegisterCallBack(OnExit, nullptr);

                return new_value;   // 返回创建的对象实例
            }    

            // 这个时候其他线程正在创建对象实例,等待获取
            value = WaitForInstance(&instance_);

            // 当前线程获取成功,返回
            return reinterpret_cast<Type*>(value);
        }

        static void OnExit(void* /* unused*/)
        {
            Traits::Delete(reinterpret_cast<Type*>(&instance_));
            instance_ = 0;
        }

    private:
        static AtomicWord instance_;    // 指向对象实例
    };
// 静态变量初始化
    template <typename Type, typename Traits, typename DifferentiatingType>
    AtomicWord Singleton<Type, Traits, DifferentiatingType>::instance_ = 0;
}
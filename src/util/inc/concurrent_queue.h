#pragma once

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>
#include <atomic>
#include <stdexcept>
#include <type_traits>

namespace util 
{
    constexpr size_t kDefaultCapacity = 1024;

    template <typename T>
    struct AlignedAllocator
    {
        using value_type = T;

        T* Allocate(size_t n)
        {
            if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
                throw std::bad_array_new_length();
            T* ret;
            if (!posix_memalign(reinterpret_cast<void**>(&ret), alignof(T), sizeof(T) * n))
                throw std::bad_alloc();

            return ret;
        }

        void Deallocate(T* v, size_t size)
        {
            if (v == nullptr)
                return;
            free(v);
        }
    };


    template <typename T>
    struct Slots
    {
        ~Slots()
        {
            if (turn_ & 1)
                Destroy();
        }

        template <typename... Args>
        void Construct(Args&&... args) noexcept
        {
            new (&storage_) T(std::forward<Args>(args)...);
        }

        void Destroy() noexcept
        {
            reinterpret_cast<T*>(&storage_)->~T();
        }

        T&& Move() noexcept
        {
            return reinterpret_cast<T&&>(storage_);
        }

        std::atomic<size_t> turn_ = { 0 };
        typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
    };


    template <typename T, typename Allocator = AlignedAllocator<Slots<T>>>
    class ConcurrentQueue
    {
    public:
        ConcurrentQueue(size_t capacity = kDefaultCapacity, const Allocator& allocator = Allocator())
            : capacity_(capacity), allocator_(allocator)
        {
            if (capacity < 1)
                throw std::invalid_argument("capacity < 1");

            slots_ = allocator_.Allocate(capacity + 1);
            if (reinterpret_cast<size_t>(slots_) % alignof(Slots<T>) != 0)
            {
                allocator_.Deallocate(slots_, capacity + 1);
                throw std::bad_alloc();
            }

            for (size_t i = 0; i < capacity_; i++)
                new (&slots_[i]) Slots<T>();
        }

        ~ConcurrentQueue() noexcept
        {
            for (size_t i = 0; i < capacity_; i++)
                slots_[i].~T();
            allocator_.Deallocate(slots_, capacity_ + 1);
        }

        ConcurrentQueue(const ConcurrentQueue&) = delete;
        ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

// 压入操作
        template <typename... Args>
        void Emplace(Args&&... args) noexcept
        {
            auto head = head_.fetch_add(1);
            auto& slot = slots_[Index(head)];
            while (Turn(head) * 2 != slot.turn_.load(std::memory_order_acquire));

            slot.Construct(std::forward<Args>(args)...);
            slot.turn_.store(Turn(head) * 2 + 1, std::memory_order_release);
        }
        
        template <typename... Args>
        bool TryEmplace(Args&&... args) noexcept
        {
            auto head = head_.load(std::memory_order_acquire);
            while (true)
            {
                auto& slot = slots_[Index(head)];
                if (Turn(slot.turn_) * 2 == slot.turn_.load(std::memory_order_acquire))
                {
                    if (head_.compare_exchange_strong(head, head + 1))
                    {
                        slot.Construct(std::forward<Args>(args)...);
                        slot.turn_.store(Turn(head) * 2 + 1, std::memory_order_release);
                        return true;
                    }
                }
                else 
                {
                    auto const prev_head = head;
                    head = head_.load(std::memory_order_acquire);
                    if (head == prev_head)
                        return false;
                }
            }
        }

        void Push(const T& val) noexcept
        { Emplace(val); }

        template <typename P, 
            typename = typename std::enable_if<std::is_nothrow_constructible<T, P&&>::value>::type>
        void TryPush(const T& val) noexcept
        { TryEmplace(val); }


// 弹出操作
        void Pop(T& val) noexcept
        {
            const auto tail = tail_.fetch_add(1);
            auto& slot = slots_[Index(tail)];
            while (Turn(tail) * 2 + 1 != slot.turn_.load(std::memory_order_acquire));

            val = slot.Move();
            slot.Destroy();
            slot.turn_.store(Turn(tail) * 2 + 2, std::memory_order_release);
        }

        bool TryPop(T& val) noexcept
        {
            auto tail = tail_.load(std::memory_order_acquire);
            while (true)
            {
                auto& slot = slots_[Index(tail)];
                if (Turn(tail) * 2 + 1 == slot.turn_.load(std::memory_order_acquire))
                {
                    if (tail_.compare_exchange_strong(tail, tail + 1))
                    {
                        val = slot.Move();
                        slot.Destroy();
                        slot.turn_.store(Turn(tail) * 2 + 2, std::memory_order_release);
                        return true;
                    }
                }
                else 
                {
                    auto const prev_tail = tail;
                    tail = tail_.load(std::memory_order_acquire);
                    if (prev_tail == tail)
                        return false;
                }
            }
        }

        std::ptrdiff_t Size() const noexcept
        { return static_cast<ptrdiff_t>(head_.load(std::memory_order_acquire) - 
                                        tail_.load(std::memory_order_acquire)); }
        
        bool Empty() const noexcept
        { return Size() <= 0; }

    private:    
        constexpr size_t Index(size_t i) const noexcept
        { return i % capacity_; }

        constexpr size_t Turn(size_t i) const noexcept
        { return i / capacity_; }
    private:
        std::atomic<size_t> head_;
        std::atomic<size_t> tail_;
        size_t capacity_;
        Allocator allocator_;
        Slots<T>* slots_;
    };


}
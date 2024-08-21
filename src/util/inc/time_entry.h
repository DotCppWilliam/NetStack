#pragma once

#include "def.h"

#include <algorithm>
#include <bits/types/struct_timeval.h>
#include <ctime>
#include <iterator>
#include <sys/time.h>
#include <iostream>
#include <vector>


#define TIME_FMT_CNT (7) // 时间格式的个数: 年、月、日、时、分、秒、纳秒. 这个一共有7个


namespace netstack 
{
    /**
    * @brief 管理时间戳timeval
    * 
    * @tparam T 
    */
    struct TimeEntry
    {
    public:
        TimeEntry()
        { 
            value_ =  {-1, -1}; // 初始化 秒和微妙为-1
        }

        // 拷贝构造函数
        TimeEntry(const TimeEntry& val)
        { 
            value_ = val.value_;
        }

        // 设置时间戳
        TimeEntry(const timeval& val) noexcept
        { value_ = val; }

        // 拷贝运算符
        TimeEntry& operator=(const TimeEntry& val) noexcept
        { 
            value_ = val.value_; 
            return *this;
        }

        // 拷贝运算符,设置时间戳
        TimeEntry& operator=(const timeval& val)
        { 
            value_ = val;
            return *this;
        }


        friend TimeEntry operator-(const TimeEntry& lhs, const TimeEntry& rhs)
        {
            long sum = lhs.value_.tv_sec * TIME_BASE_NS + 
                lhs.value_.tv_usec + rhs.value_.tv_sec * TIME_BASE_NS +
                rhs.value_.tv_usec;
            timeval ans = { sum / TIME_BASE_NS, sum % TIME_BASE_NS };
            TimeEntry time_ans(ans);
            return time_ans;
        }

        friend const TimeEntry operator+(const TimeEntry& lhs, const TimeEntry& rhs) 
        {
            long sum = lhs.value_.tv_sec * TIME_BASE_NS 
                + lhs.value_.tv_usec 
                + rhs.value_.tv_sec*TIME_BASE_NS 
                + rhs.value_.tv_usec;
            timeval ans = { sum / TIME_BASE_NS, sum % TIME_BASE_NS };
            TimeEntry time_ans(ans);
            return time_ans;           
        }

        friend TimeEntry operator%(const TimeEntry& lhs, const TimeEntry& rhs)
        {
            long lsum = lhs.value_.tv_sec * TIME_BASE_NS + rhs.value_.tv_usec;
            long rsum = rhs.value_.tv_sec * TIME_BASE_NS + rhs.value_.tv_usec;
            timeval ans = { lsum % rsum / TIME_BASE_NS, lsum % rsum % TIME_BASE_NS };
            TimeEntry time_ans(ans);
            return time_ans;
        }

        friend TimeEntry operator*(const TimeEntry& lhs, const long& rhs)
        {
            long lsum = lhs.value_.tv_sec * TIME_BASE_NS + lhs.value_.tv_usec;
            timeval ans = { lsum * rhs / TIME_BASE_NS, lsum * rhs % TIME_BASE_NS };
            TimeEntry time_ans(ans);
            return time_ans;
        }

        friend const long operator/(const TimeEntry& lhs, const TimeEntry& rhs)
        {
            long lsum = lhs.value_.tv_sec * TIME_BASE_NS + lhs.value_.tv_usec;
            long rsum = rhs.value_.tv_sec * TIME_BASE_NS + rhs.value_.tv_usec;
            return lsum / rsum;
        }

        friend bool operator<(const TimeEntry& lhs, const TimeEntry& rhs)
        {
            long lsum = lhs.value_.tv_sec * TIME_BASE_NS + rhs.value_.tv_usec;
            long rsum = rhs.value_.tv_sec * TIME_BASE_NS + rhs.value_.tv_usec;
            return lsum < rsum;
        }

        friend bool operator>(const TimeEntry& lhs, const TimeEntry& rhs) 
        {
            long lsum = lhs.value_.tv_sec * TIME_BASE_NS + lhs.value_.tv_usec;
            long rsum = rhs.value_.tv_sec * TIME_BASE_NS + rhs.value_.tv_usec;
            return lsum > rsum;
        }

        friend
        bool operator>=(const TimeEntry& lhs, const TimeEntry& rhs) 
        {
            long lsum = lhs.value_.tv_sec * TIME_BASE_NS + lhs.value_.tv_usec;
            long rsum = rhs.value_.tv_sec * TIME_BASE_NS + rhs.value_.tv_usec;
            return lsum >= rsum;
        }

        friend
        bool operator!=(const TimeEntry& lhs, const TimeEntry& rhs) 
        {
            long lsum = lhs.value_.tv_sec * TIME_BASE_NS + lhs.value_.tv_usec;
            long rsum = rhs.value_.tv_sec * TIME_BASE_NS + rhs.value_.tv_usec;
            return lsum != rsum;
        }

        friend std::ostream & operator<<(std::ostream & os,const TimeEntry & val) 
        {
            os << val.value_.tv_sec << "s " << val.value_.tv_usec << "us";
            return os;
        }

        // 获取时间戳
        timeval GetTimeval()
        { return value_; }

        // 获取当前时间戳并保存下来
        void GetTimeofday()
        { gettimeofday(&value_, nullptr); }

    public:
        timeval value_;   // 时间戳,默认为timeval类型(秒,微妙)
    };


    /**
    * @brief 管理多个延迟时间
    * 
    */
    struct TimeExpiration
    {
        using ValueType = TimeEntry;

        TimeExpiration() = default;
        TimeExpiration(const TimeExpiration& timer_expiration)
        {
            timevars_ = timer_expiration.timevars_;
        }

        /**
        * @brief 添加延迟时间
        * 
        * @tparam T 
        * @param time 
        */
        template <typename T>
        void SetTime(T&& time)
        { timevars_.push_back(time); }

        /**
        * @brief 添加多个定时时间
        * 
        * @tparam T 
        * @tparam Args 
        * @param head 
        * @param args 
        */
        template <typename T, typename... Args>
        void SetTime(T&& head, Args&&... args)
        {
            timevars_.push_back(head);
            SetTime(args...);
        }

        /**
        * @brief 添加多个延迟时间
        * 
        * @tparam Args 
        * @param args 
        */
        template <typename... Args>
        TimeExpiration(Args&&... args)
        { SetTime(std::forward<Args>(args)...); }

        /**
        * @brief 获取延迟时间.将年月日这种时间格式转换成 秒和微妙
        * 
        * @return const timeval 
        */
        const timeval GetTimeval()
        {
            // 存放的具体时间 (年月日时分秒) 这样的
            std::vector<long> timevars(TIME_FMT_CNT, 0);
            int i = 0;
            // 成员变量获取延迟时间
            std::for_each(std::begin(timevars_), std::end(timevars_), 
            [&](long& x){
                timevars[i] = x;
                i++;
            });
            
            // 转换tm
            struct tm tmp_tm = {
                static_cast<int>(timevars[5]),           // second
                static_cast<int>(timevars[4]),           // minute
                static_cast<int>(timevars[3]),          // hour
                static_cast<int>(timevars[2]),          // day
                static_cast<int>(timevars[1]) - 1,       // mon
                static_cast<int>(timevars[0]) - 1900    // year
            };

            time_t time_sec = mktime(&tmp_tm);
        #ifdef ENABLE_DEBUG
            struct tm* local_time = localtime(&time_sec);
            char buf[256] = "";
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", local_time);
            printf("%s\n", buf);
        #endif
            return timeval({ time_sec, timevars[6] });
        }
        
        /**
        * @brief 
        * 
        * @return const ValueType 
        */
        const ValueType GetTimeEntry()
        {
            timeval tm_val = GetTimeval();
            return TimeEntry(tm_val);
        }
        
        std::vector<long> timevars_;    // 保存 年、月、日、时、分、秒具体时间的延迟时间
    };

}
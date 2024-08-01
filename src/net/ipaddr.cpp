#include "ipaddr.h"
#include "net_err.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace net 
{
    NetErr_t IpAddr2Str(IpAddr& src_addr, std::string& dst_str)
    {
        if (src_addr.addr32_ == 0)
            return NET_ERR_PARAM;
        
        std::string ret;
        char buf[32];
        for (int i = 0; i < 4; i++)
        {
            if (src_addr.addr8_arr_[i] < 0)
                return NET_ERR_PARAM;
            memset(buf, '\0', sizeof(buf));
            snprintf(buf, sizeof(buf), "%d", src_addr.addr8_arr_[i]);
            ret += buf;
            if (i <= 2)
                ret += ".";
        }
        dst_str = ret;
        return NET_ERR_OK;
    }


    NetErr_t Str2IpAddr(std::string& src_str, IpAddr& dst_addr)
    {
        if (src_str.empty())
            return NET_ERR_PARAM;
        
        IpAddr addr;
        std::string tmp;
        std::string::size_type ret;
        int index = 0;

        while ((ret = src_str.find(".")) != std::string::npos)
        {   
            tmp = src_str.substr(0, ret);
            src_str = src_str.substr(ret + 1, src_str.size());
            for (int i = 0; i < tmp.size(); i++)
            {
                if (!isalnum(tmp[i]))
                    return NET_ERR_PARAM;
            }
            if (tmp.size() > 3)
                return NET_ERR_PARAM;

            int num = atoi(tmp.c_str());
            if (num < 0)
                return NET_ERR_PARAM;
            addr.addr8_arr_[index++] = num;
        }
        if (!src_str.empty() && (ret = src_str.find(".")) == std::string::npos)
        {
            int num = atoi(src_str.c_str());
            addr.addr8_arr_[index++] = num;
            src_str.clear();
        }

        if (!src_str.empty() || index != 4)
            return NET_ERR_PARAM;
        
        dst_addr = addr;
        return NET_ERR_OK;
    }
}
#include "sys_plat.h"
#include "packet_buffer.h"
#include "pcap.h"
#include "net_err.h"
#include "util.h"

#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <exception>
#include <unordered_map>
#include <netpacket/packet.h>

namespace netstack 
{
    std::vector<NetInfo*> kDevices;


    ////////////////////////////////////////////////// PcapNICDriver
    PcapNICDriver::~PcapNICDriver()
    {
        for (auto& device : kDevices)
            pcap_close(device->device);
    }


    PcapNICDriver::PcapNICDriver()
    {
        bool ret = OpenAllDefaultDevice();
        if (!ret)
            throw std::runtime_error("open network card failed");
    }

    bool PcapNICDriver::OpenAllDefaultDevice()
    {
        pcap_if_t* all_devices, *dev;
        pcap_t* handle;
        char err_buf[PCAP_ERRBUF_SIZE];
        NetInfo* info = nullptr;
        std::unordered_map<std::string, uint64_t> mac_map;

        if (pcap_findalldevs(&all_devices, err_buf) == -1)
        {
            // 日志: TODO: 
            return false;
        }
        
        struct ifaddrs* ifaddr, *ifa;
        if (getifaddrs(&ifaddr) == -1)
        {
            printf("getifaddrs failed");
            return false;
        }

        
        // 逐个打开网络接口
        for (dev = all_devices; dev != nullptr; dev = dev->next)
        {
            for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr == nullptr)   
                    continue;
                
                // 记录网卡mac地址
                if (ifa->ifa_addr->sa_family == AF_PACKET)
                {
                    if (mac_map.find(ifa->ifa_name) == mac_map.end())
                    {
                        struct sockaddr_ll* sll = (struct sockaddr_ll*)ifa->ifa_addr;
                        uint64_t mac = 0;
                        memcpy(&mac, sll->sll_addr, sizeof(char) * 6);
                        mac_map.insert( { ifa->ifa_name, mac });
                    }
                }
                
                // 设置网卡ip地址、名字、子网掩码等信息
                if (!strcmp(dev->name, ifa->ifa_name) && ifa->ifa_addr->sa_family == AF_INET)
                {
                    handle = pcap_open_live(dev->name, std::numeric_limits<int>::max(), 1, 1000, err_buf);
                    if (handle == nullptr)
                    {
                        // TODO: 日志输出: 打开网卡失败
                        return false;
                    }

                    info = new NetInfo;
                // 记录ip地址
                    struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
                    *(uint32_t*)info->ip = addr->sin_addr.s_addr;
                // 记录子网掩码
                    struct sockaddr_in* netmask = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_netmask);
                    *(uint32_t*)info->netmask = netmask->sin_addr.s_addr;
                // 设置网卡类型    
                    info->SetType(dev->name);
                // 记录网卡名称
                    info->name = dev->name;
                // 设置网卡操作的指针
                    info->device = handle;
                    kDevices.push_back(info);   
                    if (info->type == NETIF_TYPE_LOOP)
                        loop_device_ = kDevices.back();
                }
            }
        }
        for (auto& device : kDevices)
        {
            uint64_t mac = mac_map.find(device->name)->second;
            memcpy(device->mac, &mac, sizeof(char) * 6);
        }
        
        return true;
    }


    /**
    * @brief 获取IP地址的设备名称
    * 
    * @param name_buf 找到的设备名称
    * @return bool true: 成功找到, false: 查找失败 
    */
    bool PcapNICDriver::FindDevice(const char* ip, char* name_buf)
    {
        struct in_addr dest_ip;
        inet_pton(AF_INET, ip, &dest_ip);

        // 获取所有接口列表
        char err_buf[PCAP_BUF_SIZE];
        pcap_if_t* pcap_if_list = nullptr;
        // 扫描系统上的网络接口设备,将其信息存储在一个链表中
        // 存储的信息包含 接口的名称、描述、地址等
        int err = pcap_findalldevs(&pcap_if_list, err_buf);
        if (err < 0)
        {
            fprintf(stderr, "扫描网卡失败 %s\n", err_buf);
            pcap_freealldevs(pcap_if_list);
            return false;
        }

        // 遍历列表
        pcap_if_t* item;
        for (item = pcap_if_list; item != nullptr; item = item->next)
        {
            if (item->addresses == nullptr) 
                continue;
            // 查找地址
            for (struct pcap_addr* pcap_addr = item->addresses; pcap_addr != nullptr;
                pcap_addr = pcap_addr->next)
            {
                // 检查ipv4地址类型
                struct sockaddr* sock_addr = pcap_addr->addr;
                if (sock_addr->sa_family != AF_INET)
                    continue;
                // 地址相同则返回
                struct sockaddr_in* curr_addr = reinterpret_cast<struct sockaddr_in*>(sock_addr);
                if (curr_addr->sin_addr.s_addr == dest_ip.s_addr)
                {
                    strcpy(name_buf, item->name);
                    pcap_freealldevs(pcap_if_list);
                    return true;
                }
            }
        }
        pcap_freealldevs(pcap_if_list);
        return false;
    }

    /**
    * @brief 输出系统上所有网卡设备信息
    * 
    * @return bool 
    */
    bool PcapNICDriver::ShowList()
    {
        char err_buf[PCAP_ERRBUF_SIZE];
        pcap_if_t* pcapif_list = nullptr;
        int count = 0;

        // 查找所有网络接口
        int err = pcap_findalldevs(&pcapif_list, err_buf);
        if (err < 0)
        {
            fprintf(stderr, "扫描网卡失败 %s\n", err_buf);
            pcap_freealldevs(pcapif_list);
            return false;
        }
        printf("网卡列表: \n"); 

        // 遍历所有可用接口,输出其信息
        for (pcap_if_t* item = pcapif_list; item != nullptr; item = item->next)
        {
            if (item->addresses == nullptr)
                continue;
            for (struct pcap_addr* pcap_addr = item->addresses; pcap_addr != nullptr;
                pcap_addr = pcap_addr->next)
            {
                char str[INET_ADDRSTRLEN];
                struct sockaddr_in* ip_addr;

                struct sockaddr* sockaddr = pcap_addr->addr;
                if (sockaddr->sa_family != AF_INET)
                    continue;

                ip_addr = (struct sockaddr_in*)sockaddr;
                char* name = item->description;
                if (name == nullptr)
                    name = item->name;

                printf("%d: IP: {%s} name: [%s]\n", count++, 
                    name ? name : "unknown", inet_ntop(AF_INET, &ip_addr->sin_addr, str, sizeof(str)));
                break;
            }
        }
        pcap_freealldevs(pcapif_list);

        if ((pcapif_list == nullptr) || (count == 0))
        {
            fprintf(stderr, "网卡找不到,请检查系统配置\n");
            return false;
        }

        return true;
    }


    /**
    * @brief 
    * 
    * @param mac_addr 
    * @return pcap_t* 
    */
    NetErr_t PcapNICDriver::DeviceOpen(const char* ip, const uint8_t* mac_addr)
    {
        pcap_t* device = nullptr;
        NetInfo* info;
        // 利用上层传来的ip地址
        char name_buf[256];
        if (!FindDevice(ip, name_buf))
        {
            fprintf(stderr, "pcap查找失败: 没有网卡包含此IP: {%s}\n", ip);
            ShowList();
            return NET_ERR_IO;
        }
        
        // 根据名称获取ip地址、掩码等
        char err_buf[PCAP_ERRBUF_SIZE];
        bpf_u_int32 mask;   // 子网掩码
        bpf_u_int32 net;    // 网络号
        if (pcap_lookupnet(name_buf, &net, &mask, err_buf) == -1)
        {
            printf("pcap_loopupnet error: 没有网卡: %s\n", name_buf);
            net = 0;
            mask = 0;
        }

        // 打开设备
        device = pcap_create(name_buf, err_buf);
        if (device == nullptr)
        {
            fprintf(stderr, "pcap_create: 创建pcap失败 %s\n网卡名: %s", err_buf, name_buf);
            fprintf(stderr, "使用以下内容:\n");
            ShowList();
            return NET_ERR_IO;
        }

        // 设置捕获的数据包的最大长度
        if (pcap_set_snaplen(device, 65536) != 0)
        {
            fprintf(stderr, "pcap_set_snaplen error: %s\n", pcap_geterr(device));
            return NET_ERR_IO;
        }

        // 设置网络接口为混杂模式,即捕获所有经过网络接口的数据包
        if (pcap_set_promisc(device, 1) != 0)
        {
            fprintf(stderr, "pcap_set_promisc error: %s\n", pcap_geterr(device));
            return NET_ERR_IO;
        }

        // 设置捕获数据包的超时时间
        if (pcap_set_timeout(device, 0) != 0)
        {
            fprintf(stderr, "pcap_set_timeout error: %s\n", pcap_geterr(device));
            return NET_ERR_IO;
        }

        // 设置为立即模式,即立即返回读取到的数据包,而不会等待缓冲区满或超时
        if (pcap_set_immediate_mode(device, 1) != 0)
        {
            fprintf(stderr, "pcap_set_immediate_mode error: %s\n", pcap_geterr(device));
            return NET_ERR_IO;
        }

        // 激活设备,使其处于工作状态
        if (pcap_activate(device) != 0)
        {
            fprintf(stderr, "pcap_activate error: %s\n", pcap_geterr(device));
            return NET_ERR_IO;
        }

        // 设置为非阻塞模式,使得读取数据包不会阻塞当前线程
        if (pcap_setnonblock(device, 0, err_buf) != 0)
        {
            fprintf(stderr, "pcap_setnonblock error: %s\n", pcap_geterr(device));
            return NET_ERR_IO;
        }

        // 只捕获发往本接口与广播的数据帧,相当于只处理发往这张网卡的包
        char filter_exp[256];
        struct bpf_program fp;
        // 过滤规则字符串
        snprintf(filter_exp, 256, "(ether dst %02x:%02x:%02x:%02x:%02x:%02x or ether broadcast) "
            "and (not ether src %02x:%02x:%02x:%02x:%02x:%02x)",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5],
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        
        // 编译过滤规则字符串,将其转换为pcap中使用的内部表示形式
        if (pcap_compile(device, &fp, filter_exp, 0, net) == -1)
        {
            printf("pcap_open: 无法解析过滤器 %s:%s\n", filter_exp, pcap_geterr(device));
            return NET_ERR_IO;
        }
        
        // 用于将编译好的过滤器应用
        if (pcap_setfilter(device, &fp) == -1)
        {
            printf("pcap_open: 不能安装过滤器: %s:%s\n", filter_exp, pcap_geterr(device));
            return NET_ERR_IO;
        }

        info->device = device;
        kDevices.push_back(info);   // TODO:
        return NET_ERR_OK;
    }

    /**
     * @brief 向网卡发送数据包.发送完成,将参数PacketBuffer内存释放.
     * 
     * @param netif 
     * @param pkt 
     * @return NetErr_t 
     */
    NetErr_t PcapNICDriver::SendData(pcap_t* netif, std::shared_ptr<PacketBuffer>& pkt)
    {
        if (netif == nullptr)
        {
            // TODO: 日志输出
            return NET_ERR_PARAM;
        }
        
        //u_char* data_mem = new u_char[pkt->GetDataSize()];
        int data_size = pkt->DataSize();
        unsigned char* data_mem = new unsigned char[data_size];
        pkt->Read(data_mem, data_size);   // 进行一次网络数据包拷贝
        DumpHex(data_mem, data_size);

        // 向网卡发送数据
        int i = pcap_inject(netif, data_mem, data_size);   
        if (i == -1)
        {
            const char* err_str = pcap_geterr(netif);
            printf("err: %s\n", err_str);
        }
        
        pkt.reset();    // 释放掉要发送数据包的内存
        delete [] data_mem; // 释放临时内存
        return NET_ERR_OK;
    }
    
    NetErr_t PcapNICDriver::RecvData(pcap_t* netif, std::shared_ptr<PacketBuffer>& pkt)
    {
        if (netif == nullptr)
        {
            // TODO: 日志输出
            return NET_ERR_PARAM;
        }
        struct pcap_pkthdr* pkthdr;
        const uint8_t* pkt_data;

        while (true)
        {
            int ret = pcap_next_ex(netif, &pkthdr, &pkt_data);  // 从网卡读取数据
            if (ret == 0)
                continue;
            if (ret == -1)
            {
                const char* err_str = pcap_geterr(netif);
                // TODO: 日志输出
                continue;
            }

            DumpHex(pkt_data, pkthdr->len);
            pkt = std::make_shared<PacketBuffer>(); // 创建数据包存放网络数据
            // 复制网络数据包,发生一次拷贝(无法避免,因为下一次pcap读取数据会将上一次内存清空)
            pkt->Write(pkt_data, pkthdr->len);

            // TODO: 向消息队列写入消息

            break;
        }
        return NET_ERR_OK;
    }





    ////////////////////////////////////////////////// SysSemaphore
    bool SysSemaphore::Create(int init_count)
    {
        sem_ = (sys_sem_t)malloc(sizeof(struct x_sys_sem_t));
        if (!sem_) return false;

        sem_->count_ = init_count;
        int err = pthread_cond_init(&(sem_->cond_), nullptr);
        if (err) return false;

        err = pthread_mutex_init(&(sem_->locker_), nullptr);
        if (err) return false;

        return true;
    }


    void SysSemaphore::Free()
    {
        if (!sem_) return;
        pthread_cond_destroy(&(sem_->cond_));
        pthread_mutex_destroy(&(sem_->locker_));
        free(sem_);
    }

    bool SysSemaphore::Wait(uint32_t ms)
    {
        pthread_mutex_lock(&(sem_->locker_));
        if (sem_->count_ <= 0)
        {
            int ret;
            if (ms > 0)
            {
                struct timespec ts;
                ts.tv_nsec = (ms % 1000) * 1000 * 1000L;
                ts.tv_sec = time(nullptr) + ms / 1000;
                ret = pthread_cond_timedwait(&sem_->cond_, &sem_->locker_, &ts);
                if (ret == ETIMEDOUT)
                {
                    pthread_mutex_unlock(&(sem_->locker_));
                    return false;
                }
            }
            else 
            {
                ret = pthread_cond_wait(&sem_->cond_, &sem_->locker_);
                if (ret < 0)
                {
                    pthread_mutex_unlock(&(sem_->locker_));
                    return false;
                }
            }
        }

        sem_->count_--;
        pthread_mutex_unlock(&(sem_->locker_));
        return true;
    }


    void SysSemaphore::Notify()
    {
        pthread_mutex_lock(&(sem_->locker_));
        sem_->count_++;

        // 通知线程,有资源可用
        pthread_cond_signal(&(sem_->cond_));
        pthread_mutex_unlock(&(sem_->locker_));
    }




    ////////////////////////////////////////////////// SysThread
    void CustomThread::Start()
    {   
        thread_ = std::thread(&CustomThread::ThreadFunc, this);
        thread_.detach();
    }

    void CustomThread::Stop()
    {
        if (!is_running_)
            return;
        exit_ = true;
    }

    void CustomThread::ThreadFunc()
    {
        while (!exit_)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_running_.wait(lock);
                if (func_ == nullptr)   
                    continue;
            }
            func_();
            func_ = nullptr;
        }
    }

}
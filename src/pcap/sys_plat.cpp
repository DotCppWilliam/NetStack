#include "sys_plat.h"
#include "pcap.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

////////////////////////////////////////////////// PcapNICDriver
/**
 * @brief 获取IP地址的设备名称
 * 
 * @param name_buf 找到的设备名称
 * @return bool true: 成功找到, false: 查找失败 
 */
bool PcapNICDriver::FindDevice(char* name_buf)
{
    struct in_addr dest_ip;
    inet_pton(AF_INET, ip_, &dest_ip);

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
pcap_t* PcapNICDriver::DeviceOpen(const uint8_t* mac_addr)
{
    // 利用上层传来的ip地址
    char name_buf[256];
    if (!FindDevice(name_buf))
    {
        fprintf(stderr, "pcap查找失败: 没有网卡包含此IP: {%s}\n", ip_);
        ShowList();
        return nullptr;
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
    pcap_t* pcap = pcap_create(name_buf, err_buf);
    if (pcap == nullptr)
    {
        fprintf(stderr, "pcap_create: 创建pcap失败 %s\n网卡名: %s", err_buf, name_buf);
        fprintf(stderr, "使用以下内容:\n");
        ShowList();
        return nullptr;
    }

    // 设置捕获的数据包的最大长度
    if (pcap_set_snaplen(pcap, 65536) != 0)
    {
        fprintf(stderr, "pcap_set_snaplen error: %s\n", pcap_geterr(pcap));
        return nullptr;
    }

    // 设置网络接口为混杂模式,即捕获所有经过网络接口的数据包
    if (pcap_set_promisc(pcap, 1) != 0)
    {
        fprintf(stderr, "pcap_set_promisc error: %s\n", pcap_geterr(pcap));
        return nullptr;
    }

    // 设置捕获数据包的超时时间
    if (pcap_set_timeout(pcap, 0) != 0)
    {
        fprintf(stderr, "pcap_set_timeout error: %s\n", pcap_geterr(pcap));
        return nullptr;
    }

    // 设置为立即模式,即立即返回读取到的数据包,而不会等待缓冲区满或超时
    if (pcap_set_immediate_mode(pcap, 1) != 0)
    {
        fprintf(stderr, "pcap_set_immediate_mode error: %s\n", pcap_geterr(pcap));
        return nullptr;
    }

    // 激活设备,使其处于工作状态
    if (pcap_activate(pcap) != 0)
    {
        fprintf(stderr, "pcap_activate error: %s\n", pcap_geterr(pcap));
        return nullptr;
    }

    // 设置为非阻塞模式,使得读取数据包不会阻塞当前线程
    if (pcap_setnonblock(pcap, 0, err_buf) != 0)
    {
        fprintf(stderr, "pcap_setnonblock error: %s\n", pcap_geterr(pcap));
        return nullptr;
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
    if (pcap_compile(pcap, &fp, filter_exp, 0, net) == -1)
    {
        printf("pcap_open: 无法解析过滤器 %s:%s\n", filter_exp, pcap_geterr(pcap));
        return nullptr;
    }
    
    // 用于将编译好的过滤器应用
    if (pcap_setfilter(pcap, &fp) == -1)
    {
        printf("pcap_open: 不能安装过滤器: %s:%s\n", filter_exp, pcap_geterr(pcap));
        return nullptr;
    }

    return pcap;
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
bool SysThread::Create(SysThreadFunc_t entry, void* arg)
{
    int err = pthread_create(&thread_, nullptr, (void*(*)(void*))entry, arg);
    if (err)
        return false;

    pthread_join(thread_, nullptr);
    return true;
}

void SysThread::Sleep(int ms)
{
    usleep(1000 * ms);
}


sys_thread_t SysThread::Self()
{ return pthread_self(); }


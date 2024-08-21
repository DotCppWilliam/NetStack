#include "net_pcap.h"
#include "arp.h"
#include "ether.h"
#include "net_interface.h"
#include "packet_buffer.h"
#include "pcap.h"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <sys/epoll.h>

namespace netstack
{
    std::list<NetInterface*> NetifPcap::kAllNetIf;


    NetifPcap::NetifPcap(std::list<NetInterface*> list_netif)
    {
        kAllNetIf = list_netif;
        recv_thread_.SetThreadFunc(&NetifPcap::RecvThread, this, nullptr);
    }

    NetifPcap::~NetifPcap()
    {
        recv_thread_.Stop();
        send_thread_.Stop();
        exit_ = true;
    }


    NetErr_t NetifPcap::OpenDevice()
    {
        recv_thread_.Start();
        send_thread_.Start();

        return NET_ERR_OK;
    }

    void NetifPcap::SendThread(void* arg)
    {
        pcap_t* dev = reinterpret_cast<pcap_t*>(arg);

    }

    void NetifPcap::RecvThread(void* arg)
    {
        recv_epollfd_ = epoll_create1(0);
        if (recv_epollfd_)
        {
            // TODO: 日志: 创建epoll失败
            throw std::runtime_error("create epoll failed");
        }

        struct epoll_event event;
        // 将网卡的接收事件添加到epoll中
        for (auto& netif : kAllNetIf)
        {
            int pcap_fd = pcap_fileno(netif->GetNetIf());
            event.events = EPOLLIN;
            event.data.ptr = netif;
            if (epoll_ctl(recv_epollfd_, EPOLL_CTL_ADD, pcap_fd, &event) == -1)
            {
                // TODO: 日志: 添加epoll事件失败
                throw std::runtime_error("epoll ctl failed(EPOLL_CTL_ADD)");
            }
        }
        
        std::vector<struct epoll_event> events(kAllNetIf.size());
        int nfds = 0;
        while (!exit_)
        {
            nfds = epoll_wait(recv_epollfd_, events.data(), events.size(), -1);
            if (nfds == -1)
            {
                // TODO: 日志输出
                throw std::runtime_error("epoll wait failed");
            }

            for (int i = 0; i < nfds; i++)
            {
                NetInterface* netif = reinterpret_cast<NetInterface*>(events[i].data.ptr);
                struct pcap_pkthdr* pkt_hdr;   
                const u_char* pkt_data;
                
                
                if (pcap_next_ex(netif->GetNetIf(), &pkt_hdr, &pkt_data))   // 成功捕获数据包
                {
                    // 其他超时、发生错误等情况则不考虑.只考虑成功情况.保证程序的稳定性

                    // 将网卡读取出来的数据包复制到一个数据包里
                    std::shared_ptr<PacketBuffer> pkt = std::make_shared<PacketBuffer>();
                    pkt->Write(pkt_data, pkt_hdr->len);

                    netif->PushPacket(pkt, true);  // 保存到网卡接口的输入队列中
                }
                
            }
        }
    }

    // 设置期望的arp请求包
    void NetifPcap::SetExpectedArpReply(uint32_t ipaddr, std::shared_ptr<PacketBuffer>* set_ptr)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        arp_reply_set_.insert( { ipaddr, set_ptr });
    }


    bool NetifPcap::IsExpectedArpResponse(std::shared_ptr<PacketBuffer> pkt)
    {
        EtherHdr* hdr = pkt->GetObjectPtr<EtherHdr>();
        if (hdr->protocol == ETHER_TYPE_ARP)    // 是arp包
        {
            // 获取其中的arp包
            Arp* arp = pkt->GetObjectPtr<Arp>(sizeof(EtherHdr));
            if (arp->op_code == ARP_REPLAY) // 如果是arp响应包
            {
                uint32_t ipaddr = *(uint32_t*)arp->dst_ipaddr;
                std::unique_lock<std::mutex> lock(mutex_);
                
                // 判断是否是arp模块想要的响应包
                auto it = arp_reply_set_.find(ipaddr);
                if (it != arp_reply_set_.end())
                {
                    (*it->second) = pkt;
                    arp_reply_set_.erase(it);
                    return true;
                }
            }
        }

        return false;
    }

}
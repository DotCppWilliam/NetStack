#include "net_interface.h"
#include "concurrent_queue.h"
#include "ether.h"
#include "net_init.h"
#include "net_err.h"
#include "packet_buffer.h"
#include "pcap.h"
#include "sys_plat.h"
#include <memory>

namespace netstack 
{   
    NetInterface* NetInterface::kLoopNetinterface = nullptr;

    NetInterface* GetLoopNetinterface()
    { return NetInterface::kLoopNetinterface; }



    NetInterface::NetInterface(NetInfo* netinfo, int queue_max_threshold)
        : netinfo_(netinfo), queue_max_threshold_(queue_max_threshold),
        recv_queue_(queue_max_threshold), send_queue_(queue_max_threshold)
    {
        netif_fd_ = pcap_fileno(netinfo->device);
        if (netinfo->is_default_gateway_)
            kLoopNetinterface = this;
    }

    NetInterface::~NetInterface()
    {

    }

    NetErr_t NetInterface::PushPacket(SharedPkt pkt, bool is_recv_queue, bool wait)
    {
        ConcurrentQueue<std::shared_ptr<PacketBuffer>>* conqueue;
        if (is_recv_queue)
            conqueue = &recv_queue_;
        else
            conqueue = &send_queue_;

        if (pkt->DataSize() == 0)
            return NET_ERR_PARAM;

        while (true)
        {
            bool ret = conqueue->TryPush<std::shared_ptr<PacketBuffer>>(pkt);
            if (ret == false && wait)
                continue;
            else
                return NET_ERR_FULL;
        }

        return NET_ERR_OK;
    }
    
    NetErr_t NetInterface::PopPacket(SharedPkt& pkt, bool is_recv_queue, bool wait)
    {
        ConcurrentQueue<std::shared_ptr<PacketBuffer>>* conqueue;
        if (is_recv_queue)
            conqueue = &recv_queue_;
        else
            conqueue = &send_queue_;

        if (conqueue->Empty())
            return NET_ERR_EMPTY;

        while (true)
        {
            bool ret = conqueue->TryPop(pkt);
            if (ret == false && wait)
                continue;
            else
                return NET_ERR_EMPTY;
        }

        return NET_ERR_OK;
    }

    /**
     * @brief 从网卡读取数据放入到接收队列中
     * 
     */
    bool NetInterface::NetRx()
    {
        pcap_pkthdr* hdr;
        const u_char* data;
        pcap_next_ex(netinfo_->device, &hdr, &data);

        std::shared_ptr<PacketBuffer> pkt = std::make_shared<PacketBuffer>(hdr->caplen);
        pkt->Write(data, hdr->len);

        if (recv_queue_.TryPush<std::shared_ptr<PacketBuffer>>(pkt) == false)
        {
            pkt.reset();
            return false;
        }
        return true;
    }

    /**
     * @brief 从发送队列中读取数据包通过网卡发送出去
     * 
     */
    bool NetInterface::NetTx()
    {
        if (send_queue_.Empty())    // 没有数据包可发送
            return false;

        std::shared_ptr<PacketBuffer> pkt;
        send_queue_.Pop(pkt);

        unsigned char* data = new unsigned char[pkt->DataSize()];
        pkt->Read(data, pkt->DataSize());

        // 这种情况很小,除非网卡掉了或者网卡打开失败
        if (pcap_inject(netinfo_->device, data, pkt->DataSize()) == -1)
        {
            fprintf(stderr, "pcap_inject failed: %s\n", pcap_geterr(netinfo_->device));
            delete [] data;
            pkt.reset();
            return false;
        }

        return true;
    }

    NetErr_t NetInterface::NetTx(SharedPkt pkt)
    {
        if (pkt->DataSize() == 0)
            return NET_ERR_PARAM;

        NetErr_t ret = PcapNICDriver::SendData(netinfo_->device, pkt);
        pkt.reset();

        return ret;
    }


    void HandleRecvPktCallback(NetInterface* iface)
    {
        SharedPkt pkt;
        bool ret = iface->recv_queue_.TryPop(pkt);
        if (ret == false)
            return;

        bool is = false;
        if (iface->netinfo_->name == "enp0s3")
            is = true;
        EtherPop(pkt, is);  // 交给以太网来处理,然后逐层向上传递
    }
}
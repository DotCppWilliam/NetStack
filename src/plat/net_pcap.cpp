#include "net_pcap.h"


namespace lpcap
{
    net::NetErr_t NetifPcap::Open()
    {
        recv_thread_.Create();
        send_thread_.Create();

        return net::NET_ERR_OK;
    }

    void NetifPcap::SendThread(void* arg)
    {

    }

    void NetifPcap::RecvThread(void* arg)
    {

    }

}
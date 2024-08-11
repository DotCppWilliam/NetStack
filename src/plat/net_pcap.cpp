#include "net_pcap.h"


namespace lpcap
{
    net::NetErr_t NetifPcap::OpenDevice()
    {
        recv_thread_.Start();
        send_thread_.Start();

        return net::NET_ERR_OK;
    }

    void NetifPcap::SendThread(void* arg)
    {
        
    }

    void NetifPcap::RecvThread(void* arg)
    {

    }

}
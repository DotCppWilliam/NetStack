#include "pcap.h"
#include "sys_plat.h"
#include <pcap/pcap.h>
#include <string.h>

int main()
{
    PcapNICDriver driver("192.168.56.101");
    uint8_t mac_arr[] = { 0x80, 0x00, 0x27, 0xcf, 0x16, 0xfb };
    pcap_t* pcap = driver.DeviceOpen(mac_arr);
    u_char buf[1024];
    memset(buf, 1, sizeof(buf));


    while (pcap)
    {
        struct pcap_pkthdr* pkthdr;
        const u_char* data;
        if (pcap_next_ex(pcap, &pkthdr, &data) != 1)
            continue;

        plat_printf("成功接收数据\n");
        int len = pkthdr->len > sizeof(buf) ? sizeof(buf) : pkthdr->len;
        plat_memcpy(buf, data, len);

        buf[0] = 1;
        buf[1] = 2;

        if (pcap_inject(pcap, buf, len) == -1)
        {
            plat_printf("发送数据失败: %s\n", pcap_geterr(pcap));
            return -1;
        }
        printf("发送数据成功\n");
    }
        
    


    return 0;
}
#include "sys_plat.h"
#include <pcap/pcap.h>
#include <string.h>

int main()
{
    PcapNICDriver driver("192.168.56.101");

    // 00:15:5d:fd:4b:3e
    uint8_t mac_arr[] = { 0x80, 0x00, 0x27, 0xcf, 0x16, 0xfb };

    pcap_t* pcap = driver.DeviceOpen(mac_arr);

    while  (pcap)
    {
        uint8_t buf[1024];
        memset(buf, 1, sizeof(buf));

        for (int i = 0; i < 5; i++)
        {
            if (pcap_inject(pcap, buf, sizeof(buf)) == -1)
            {
                plat_printf("发送数据包失败: %s\n", pcap_geterr(pcap));
                
                return -1;
            }
            plat_printf("成功发送数据包\n");
        }
    }

    return 0;
}
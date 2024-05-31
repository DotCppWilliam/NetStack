# 小型TCP/IP协议栈,底层使用Ncap库,基于Linux系统

## 1. 如何使用?
可以在本机使用,需要虚拟机就能实现    
需要**Virtual box**虚拟机和**Ubuntu**系统.安装好系统后，需要进行一些简单配置,下面是配置说明:    
![image](https://github.com/DotCppWilliam/NetStack/assets/129869761/8b864908-2fe4-4f70-bb79-2328a9fd5c98)   

虚拟机需要配置两个网卡:   
> ① 一个用来和宿主机进行通信,使其和宿主机在同一网段。
> ![image](https://github.com/DotCppWilliam/NetStack/assets/129869761/8c995550-f91c-465a-8e90-afb8146d9a41)
> ② 一个用来上网
> ![image](https://github.com/DotCppWilliam/NetStack/assets/129869761/6f0ac53c-4745-4786-aef5-88f0c0b738be)

*注: 如果virtual box显示不了图形的话,将显存大小设置大点*   


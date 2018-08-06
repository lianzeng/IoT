# IoT
internet of things

开发环境： 在个人电脑上安装VirualBox虚拟机+Ubuntu，再安装arm交叉编译工具(cpu芯片厂商提供或者Yocto工程下载)，然后本地虚拟机写代码编译出arm-bin，再放到开发板上运行；

1)usb to uart(seria port) need install driver: usbserial.ko && cp210x.ko, otherwise will not detect /dev/ttyUSB0;
2)this develop is on virtualbox-linux system, could generate arm bin via a cross-compile-tool.

1.对于设备文件，一定要用O_NONBLOCK方式来创建fd = open(O_RDWR | O_NONBLOCK); 避免因为读不到数据而阻塞;
2.用io multiplex异步机制来监听端口非常方便，我用的是select()来和设备通信的，只要设备上有数据就可以读取，另外设置一个buffer来接收不连续的数据，到达时间门限后再统一处理数据；
3.创建周期定时器:  fd= timerfd_create(); 然后加入select()，外面一层while循环，非常好用，不要用select的time参数充当定时器，因为会被其它的fd打断；
4.可交互的程序:  FD_SET(STDIN_FILENO, &readset); + select(maxfdp1, &readset, NULL, NULL, NULL); 这样用户可以输入指令，和程序交互，即便用户不输入，也不影响程序功能；
5.单机内进程间通信IPC用unix socket DGram, 数据格式用JSON;
6.蓝牙自动配对可以用 C程序 + ShellScript, C程序用来周期性的启动ShellScript扫描蓝牙device，Scipt脚本用expect来实现bluetoothctl的自动cmd输入。
7.嵌入式系统中的软件都使用微服务的方式开发，每个服务是一个独立的进程，都是Daemon进程，使用IPC(unix socket)通信，每个进程只做1件事，比如蓝牙自动配对服务，固件程序下载升级服务， lwm2mclient上报服务， OBD汽车数据采集服务, 秒秒测温/GPS测量/湿度测量服务；
8.网关使用Kura(开源)来管理各种外设和驱动模块，并提供UI交互，OSGI插件化机制；
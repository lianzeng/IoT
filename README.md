# IoT
internet of things

1)usb to uart(seria port) need install driver: usbserial.ko && cp210x.ko, otherwise will not detect /dev/ttyUSB0;
2)this develop is on virtualbox-linux system, could generate arm bin via a cross-compile-tool.

1.对于设备文件，一定要用O_NONBLOCK方式来创建fd = open(O_RDWR | O_NONBLOCK); 避免因为读不到数据而阻塞;
2.用io multiplex异步机制来监听端口非常方便，我用的是select()来和设备通信的，只要设备上有数据就可以读取，另外设置一个buffer来接收不连续的数据，到达时间门限后再统一处理数据；
3.创建周期定时器:  fd= timerfd_create(); 然后加入select()，非常好用，不要用select的time参数充当定时器，因为会被其它的fd打断；
4.可交互的程序:  FD_SET(STDIN_FILENO, &readset); + select(maxfdp1, &readset, NULL, NULL, NULL); 这样用户可以输入指令，和程序交互，即便用户不输入，也不影响程序功能；

//usage: sudo  ./OBD 2>&1 | tee result.txt;  or  ./OBD 1> result.txt;


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <termio.h>
#include <cstring>
#include <errno.h>
#include <sys/timerfd.h>
#include <stdint.h>
#include <sys/time.h>



extern int init_tty(int fd, const speed_t baudrate);

static char obditem[1024] = {0};

int openSerialPort(const char* portName, const speed_t baudrate)
{

    int fd = open(portName, O_RDWR);
    if(fd < 0)
    {
        printf("open %s failed!\n",portName);
        printf("usage: sudo ./OBD  \n");
        return -1;
    }

    printf("open %s ok!,ttyfd = %d \n",portName, fd);

    if(init_tty(fd, baudrate) == -1)//初始化串口
    {
        printf("init_tty failed!\n");
        return -1;
    }
    return fd;
}

int sendActiveCmd(int fd, const char* activeCmd)
{

    int wrotebytes = write(fd, activeCmd, strlen(activeCmd));
    if(wrotebytes != strlen(activeCmd))
    {
        printf("write %d bytes,failed \n", wrotebytes);
        return -1;
    }
    printf("try active with protocol: %s \n", activeCmd);
    return 0;
}

int waitObdActResp(int fd )
{
    const char* expectResp[2] = {"LINKING","OK"};
    for(int i= 0; i < sizeof(expectResp)/sizeof(expectResp[0]); i++)
    {
        char receive[100] = {0};
        int readlen = read(fd, receive, sizeof(receive) - 1);
        if (readlen >= 0)
        {
            printf("receiveMsg %d bytes: %s \n", readlen, receive);
            if(strncmp(receive, expectResp[i], strlen(expectResp[i])))//not equal
                return -1;

        }
        else
        {
            printf("error read: %d, %s \n", readlen, strerror(errno));
            return -1;
        }
    }

    return 0;
}

int activeObd(int fd)
{
    const char* activeCmd[] = {"AT+ISO14230-4ADDR", //ISO14230 协议地址激活指令
                               "AT+ISO14230-4HL", //ISO14230 协议电平激活指令
                               "AT+ISO9141-2ADDR", //ISO9141 协议地址激活指令
                               "AT+ISO15765-4STD_500K", //ISO15765 500K 标准CAN 协议激活指令
                               "AT+ISO15765-4EXT_500K", //ISO15765 500K 扩展CAN 协议激活指令
                               "AT+ISO15765-4STD_250K", //ISO15765 250K 标准CAN 协议激活指令
                               "AT+ISO15765-4EXT_250K" //ISO15765 250K 扩展CAN 协议激活指令
                               };
    const int obdProtocolNum = sizeof(activeCmd)/sizeof(activeCmd[0]);
    for(int i = 0; i < obdProtocolNum; i++)
    {
        if (sendActiveCmd(fd,activeCmd[i]) < 0) return -1;
        tcdrain(fd);//wait until cmd sent out.

        printf("wait msg from usb:\n");

        if (waitObdActResp(fd) < 0) {
            printf("active obd failure \n");
            continue;
        } else {
            printf("active obd success. \n\n");
            return 0;
        }
    }
    printf("not connect to car or not support the obd protocol currently.\n");
    return -1;
}

void readAndSendUserCmd(const char* obdfile, const int ttyfd)
{
    int cmdfd = open(obdfile, O_RDONLY);
    if(cmdfd < 0)
    {
        printf("open %s failed\n",obdfile);
        return;
    }
    printf("open %s ok, fd = %d \n",obdfile, cmdfd);

    int len = read(cmdfd, obditem, sizeof(obditem) - 1);
    if(len > 0)
    {
        printf("ready to read  obditem: %s \n", obditem);

        int wrotebytes = write(ttyfd, obditem, len);
        if(wrotebytes != len)
            printf("send obditem %d bytes,but expect %d bytes \n",wrotebytes, len);
    }
    else
        printf("%s is empty, readlen= %d \n",obdfile, len);

    close(cmdfd);
}

void receiveObdData(const int fd)
{
    char data[100] = {0};
    int len = read(fd, data, sizeof(data) - 1);
    if(len > 0)
    {
        printf("%s \n", data);
    }
}

void sendFaultCmd(int ttyfd)
{
    const char* dtc = "AT+DTC";
    int wrotebytes = write(ttyfd, dtc, strlen(dtc));
    printf("send cmd: %s \n", dtc);
    if(wrotebytes != strlen(dtc))
        printf("Error on send Fault cmd \n");
#if 0
    struct timeval now;
    if(!gettimeofday(&now, NULL))
    {
        printf("time=%ld.%06ld \n",now.tv_sec,now.tv_usec);
    }
#endif
}

void fetchObdData(const int fd, const int timerfd)
{//fetch obd data according to user cmd defined in obditem.txt

    static bool dataFaultSwitch = 1;
    fd_set readset;
    int maxfdp1 = fd > timerfd ? (fd + 1) : (timerfd + 1);
    int result;
    do{
        FD_ZERO(&readset);
        FD_SET(fd, &readset);
        FD_SET(timerfd, &readset);
        result = select(maxfdp1, &readset, NULL, NULL, NULL);
    }while(result == -1 && errno == EINTR);

    if(result > 0)
    {
        //printf("num of %d fds are ready.\n", result);
        if(FD_ISSET(timerfd,&readset))
        {
            uint64_t dummy;
            read(timerfd, &dummy, sizeof(dummy));
            if(dataFaultSwitch)
            {
                sendFaultCmd(fd);//periodically send AT+DTC to detect fault.
            }
            else
            {
                write(fd, obditem, strlen(obditem));
                printf("sending %s \n", obditem);
            }
            dataFaultSwitch ^= 0x1;
        }
        if(FD_ISSET(fd, &readset))
            receiveObdData(fd);
    }
    else if(result < 0)
    {
        printf("Error on select(): %s \n", strerror(errno));
        //exit(1);
    }
    else
    {
        printf("Error:select return 0 \n");//shoud not reach since select(timeout=infinite)
    }
}

int createObdFaultFd(const int fault_period)
{
    int faultTimerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(faultTimerfd < 0) {
        perror("timerfd_create failed");
        return -1;
    }

    struct itimerspec tmr;
    tmr.it_value.tv_sec = fault_period;
    tmr.it_value.tv_nsec = 0;
    tmr.it_interval.tv_sec = fault_period;
    tmr.it_interval.tv_nsec = 0;
    timerfd_settime(faultTimerfd, 0, &tmr, NULL);

    printf("faultfd = %d \n",faultTimerfd);
    return faultTimerfd;
}

int main(int argc, char* argv[])
{
    const char* serialPort = "/dev/ttyUSB0";

    const speed_t baudrate = 115200;//(argc > 1) ? atoi(argv[1]) : 115200;

    int ttyfd = openSerialPort(serialPort, baudrate);
    if(ttyfd < 0) return -1;

    if(activeObd(ttyfd) < 0) return -1;


    readAndSendUserCmd("obditem.txt", ttyfd);

    int faultTimerfd = createObdFaultFd(2);//period=2second(at least!)

    while(true)
    {
        fetchObdData(ttyfd, faultTimerfd);
    }

    close(ttyfd);
    return 0;
}


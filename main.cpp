//usage: sudo  ./OBD 2>&1 | tee result.txt;  or  ./OBD 1> result.txt;


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <termio.h>
#include <cstring>
#include <errno.h>
#include <sys/timerfd.h>
#include <string>
#include <stdint.h>
#include <sys/time.h>
#include <cassert>
#include "canBuffer.hpp"
#include "logger.hpp"
#include "reportJ1939.hpp"


using std::string;
//typedef unsigned int  speed_t;

extern int init_tty(int fd, const speed_t baudrate);
extern void calcJ1939ParasOverCanBuffer(const CanBusBuffer& canBusBuffer);
extern bool initObdDevice(const int ttyfd);

Logger g_logger;
CanBusBuffer g_canBusBuffer;
J1939Reports g_j1939Report = {0};

int openSerialPort(const char* portName, const speed_t baudrate)
{

    int fd = open(portName, O_RDWR|O_NONBLOCK);
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


int waitObdActResp(int fd )
{
    for(;;)
    {
        char receive[255] = {0};
        int readlen = read(fd, receive, sizeof(receive) - 1);
        if (readlen >= 0)
        {
            printf(" %s \n", receive);
            if(strstr(receive,"ok") != NULL) //contain "ok"
                break;


        }
    }

    return 0;
}


void printOnReceive(const int fd)
{
    static char buff[50];
    memset(buff, 0, sizeof(buff));
    int len = ::read(fd,buff,sizeof(buff)-1);
    printf(" received %d bytes: %s \n",len, buff);
}

void readInputThenSendToObd(const int ifd, const int ofd)
{
    static char buff[30];
    memset(buff, 0, sizeof(buff));
    int len = ::read(ifd,buff,sizeof(buff)-1);
    printf("user input %d bytes: %s \n",len, buff);
    buff[len-1]='\r';//ELM327 cmd terminate with \r
    if(::write(ofd, buff, len) != len)
        printf("send cmd error!\n");
}

void fetchObdData(const int fd, const int timerfd)
{//fetch obd data according to user cmd defined in obditem.txt


    fd_set readset;
    int maxfdp1 = fd > timerfd ? (fd + 1) : (timerfd + 1);
    int result;
    do{
        FD_ZERO(&readset);
        FD_SET(fd, &readset);
        FD_SET(timerfd, &readset);
        FD_SET(STDIN_FILENO, &readset);//debug only,interactive input
        result = select(maxfdp1, &readset, NULL, NULL, NULL);
    }while(result == -1 && errno == EINTR);

    if(result > 0)
    {
        //printf("num of %d fds are ready.\n", result);

        if(FD_ISSET(timerfd,&readset))
        {
            uint64_t dummy;
            read(timerfd, &dummy, sizeof(dummy));
            calcJ1939ParasOverCanBuffer(g_canBusBuffer);
            g_canBusBuffer.clear();
        }

        if(FD_ISSET(fd, &readset))
        {
            printOnReceive(fd);
            //g_canBusBuffer.receiveData(fd);//TODO:temporary comment out
            if(g_canBusBuffer.isFull())
            {
                calcJ1939ParasOverCanBuffer(g_canBusBuffer);
                g_canBusBuffer.clear();
            }
        }
        if(FD_ISSET(STDIN_FILENO, &readset))//debug
        {
            readInputThenSendToObd(STDIN_FILENO, fd);
        }
    }
    else if(result < 0)
    {
        printf("Error on select(): %s \n", strerror(errno));
        //exit(1);
    }
    else
    {
        printf("Error:select return 0 \n");//shoud not reach since select(timeout=NULL)
    }
}

int createTimerFd(const int periodsec,const int nsec)
{
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(timerfd < 0) {
        perror("timerfd_create failed");
        return -1;
    }

    struct itimerspec tmr;
    tmr.it_value.tv_sec = periodsec;
    tmr.it_value.tv_nsec = nsec;
    tmr.it_interval.tv_sec = periodsec;//periodically
    tmr.it_interval.tv_nsec = nsec;
    timerfd_settime(timerfd, 0, &tmr, NULL);

    //printf("faultfd = %d \n",timerfd);
    return timerfd;
}


int startCanbusListen(int fd)
{
    const char* canFilterSettings = "STD0250000 000 000 000 000";//only for taobao analysis tool.
    printf("config can filter: %s \n", canFilterSettings);

    int wrotebytes = write(fd, canFilterSettings, strlen(canFilterSettings));
    if(wrotebytes != strlen(canFilterSettings)) {
        printf("send cmd error. \n");
        return -1;
    }
    return waitObdActResp(fd);
}

int main(int argc, char* argv[])
{
    const char* logfile = (argc >1) ? argv[1] : NULL;
    g_logger = Logger(logfile);

    const char* serialPort = "/dev/rfcomm0";

    const speed_t baudrate = 115200;

    int ttyfd = openSerialPort(serialPort, baudrate);//STDIN_FILENO;
    if(ttyfd < 0) return -1;

    if(! initObdDevice(ttyfd)) return -1;

    //if(startCanbusListen(ttyfd) < 0) return -1;
    int timerfd = createTimerFd(1,0);//period=1second to calulate 1939 params


    while(true)
    {
        fetchObdData(ttyfd, timerfd);
        g_logger.report(g_j1939Report);//report to IMPACT or file
    }

    //close(ttyfd);
    g_logger.finish();
    return 0;
}


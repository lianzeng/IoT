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
#include <sys/shm.h>
#include "shareMemData.h"

using std::string;
//typedef unsigned int  speed_t;

extern int init_tty(int fd, const speed_t baudrate);
extern void calcJ1939ParasOverCanBuffer(const CanBusBuffer& canBusBuffer);
extern bool initObdDevice(const int ttyfd);
extern void fetchObdData(const int ttyfd, const int timerfd);
extern bool initiCarDevice_J1939(const int ttyfd);
extern void fetchJ1939Data_auto(const int ttyfd, const int timerfd);
extern int createUnixSocket();

Logger g_logger;
CanBusBuffer g_canBusBuffer;
ObdDataInfo g_obdDataFetched = {0};//report to lwm2m
int    g_obdFd = -1;

int openSerialPort(const char* portName, const speed_t baudrate)
{

    int fd = open(portName, O_RDWR|O_NONBLOCK);
    if(fd < 0)
    {
        printf("open %s failed!\n",portName);
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
    printf("can tool resp ok! \n");
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

void* createShareMemory(key_t k, size_t size)
{
    //创建共享内存
    int shmid = shmget(k, size, 0666|IPC_CREAT);
    if(shmid == -1)
    {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }
    //将共享内存连接到当前进程的地址空间
    void* shm = shmat(shmid, 0, 0);
    if(shm == (void*)-1)
    {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    printf("\n shareMemory attached at %p \n", shm);
    memset(shm, 0, size);
    return shm;
}


int g_fdIpc = -1;




void fetchJ1939Data_interactive(const int fd, const int timerfd)
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
            //printOnReceive(fd);
            g_canBusBuffer.receiveData(fd);//TODO:temporary comment out
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




bool tryInitialObdDevice(const int ttyfd)
{
    const int MAX_TRY_NUM = 3;
    for(int i = 0; i<MAX_TRY_NUM; i++)
    {
        if(initObdDevice(ttyfd)) //initiCarDevice_J1939
            return true;
    }
    return false;
}

int tryOpenAndInitialObd()
{
    const speed_t baudrate = 115200; //obd  device baud rate
    const char* serialPort = "/dev/rfcomm0";

    const int ttyfd = openSerialPort(serialPort, baudrate);//STDIN_FILENO;
    if(ttyfd > 0)//obd device is ready
    {
        printf("obd is ready \n");
        if(tryInitialObdDevice(ttyfd)) //initiCarDevice_J1939(ttyfd)
        {
            return ttyfd;
        }
        else
        {
            printf("error:obd initial failure after multiple try\n");//should not run to this branch in most case
            return -1;
        }
    }
    else {
        printf("obd is not ready.\n");
        return -1;
    }
}

bool checkObdStatus(const int ttyfd)
{
    const char* dummyCmd = "010C\r";//"010C" is for 15765, j1939 may need another cmd
    if(::write(ttyfd, dummyCmd,  strlen(dummyCmd)) != strlen(dummyCmd))//send dummyCmd to check if obd is able to write,if can't write,need re-connect blue
    {
        printf("obd disable now !\n");
        return false;
    }
    return  true;
}

void fetchDataUntilObdDisable(const int ttyfd)
{

    const unsigned long ms = 1.0e6;//1ms = 1.0e6 ns
    static int timerfd = createTimerFd(0,100*ms);//the report interval = reportItemNum * timerFd;

    if(ttyfd > 0)
    {
        while(true)
        {

            fetchObdData(ttyfd, timerfd); //send to lwm2m
            //fetchJ1939Data_auto(ttyfd, timerfd);
            //fetchJ1939Data_interactive(ttyfd, timerfd);//support interactive input
            //g_logger.report(g_obdDataFetched);//log to file

            if(!checkObdStatus(ttyfd)) break;
        }
    }

}



void periodCheckObdAndFetchData(const int obdCheckTimerfd)
{
    fd_set readset;
    int maxfdp1 = (obdCheckTimerfd + 1);
    int result;
    do {
        FD_ZERO(&readset);
        FD_SET(obdCheckTimerfd, &readset);
        result = select(maxfdp1, &readset, NULL, NULL, NULL);
    } while (result == -1 && errno == EINTR);

    if(result > 0)
    {
        uint64_t dummy;
        read(obdCheckTimerfd, &dummy, sizeof(dummy));//tiemrfd must read

        if(true || checkObdStatus(g_obdFd)) //obd device ready
        {
            fetchDataUntilObdDisable(1);//while-loop, break if obd become disable
        }
        else
        {
            if((g_obdFd = tryOpenAndInitialObd()) > 0)
            {
                fetchDataUntilObdDisable(g_obdFd);
            }

        }
    }

}




int main(int argc, char* argv[])
{
    //const char* logfile = (argc >1) ? argv[1] : NULL;
    //g_logger = Logger(logfile);


    g_fdIpc = createUnixSocket();

    const int obdCheckTimerfd = createTimerFd(10,0);//check obd device statue change, power on/off

    while(true)
    {
        periodCheckObdAndFetchData(obdCheckTimerfd);
    }


    //close(ttyfd);
    //g_logger.finish();
    return 0;
}


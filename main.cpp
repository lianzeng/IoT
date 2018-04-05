//usage:  ./OBD 2>&1 | tee result.txt;  or  ./OBD 1> result.txt;


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <termio.h>
#include <cstring>
#include <errno.h>


//typedef unsigned int  speed_t;

extern int init_tty(int fd, const speed_t baudrate);


int openSerialPort(const char* portName, const speed_t baudrate)
{

    int fd = open(portName, O_RDWR);
    if(fd < 0)
    {
        printf("open %s failed!\n",portName);
        printf("usage: sudo ./OBD  \n");
        return -1;
    }

    printf("open %s ok!\n",portName);

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
            printf("active obd success. \n");
            return 0;
        }
    }
    printf("not connect to car or not support the obd protocol currently.\n");
    return -1;
}

void readAndSendUserCmd(const int cmdfd, const int ttyfd)
{
    char cmd[100] = {0};
    int len = read(cmdfd, cmd, sizeof(cmd) - 1);
    if(len > 0)
    {
        printf("send user cmd: %s \n", cmd);
        if(!strncmp(cmd, "QUIT",4) || !strncmp(cmd, "quit",4))
            exit(0);
        int wrotebytes = write(ttyfd, cmd, len);
        if(wrotebytes != len)
            printf("send cmd failed \n");
    }
    else
        printf("read user cmd failed, len= %d \n", len);
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

void fetchObdData(const int fd, const int cmdfd)
{
    //fetch obd data according to user cmd
    fd_set readset;
    int maxfd = fd > cmdfd ? (fd + 1) : (cmdfd + 1);
    int result;
    do{
        FD_ZERO(&readset);
        FD_SET(fd, &readset);
        FD_SET(cmdfd,&readset);
        result = select(maxfd, &readset, NULL, NULL, NULL);
    }while(result == -1 && errno == EINTR);

    if(result > 0)
    {
        if(FD_ISSET(cmdfd,&readset))
        {
            readAndSendUserCmd(cmdfd, fd);
            //tcflush(fd, TCIFLUSH);
        }
        if(FD_ISSET(fd, &readset))
            receiveObdData(fd);
    }
    else if(result < 0)
    {
        printf("Error on select(): %s \n", strerror(errno));
        exit(1);
    }
    else
    {
        printf("error:select return 0 \n");//shoud not reach since select(timeout=infinite)
    }

}

int main(int argc, char* argv[])
{
    const char* serialPort = "/dev/ttyUSB0";

    speed_t baudrate = (argc > 1) ? atoi(argv[1]) : 115200;

    int fd = openSerialPort(serialPort, baudrate);
    if(fd < 0) return -1;

    if(activeObd(fd) < 0) return -1;

    while(true)
    {
        fetchObdData(fd, STDIN_FILENO);
    }

    close(fd);
    return 0;
}


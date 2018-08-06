//
// Created by lianzeng on 18-3-29.
//

#include <stdio.h>
#include <string.h>
#include <termios.h>


speed_t Convert2StandBaudRate(const speed_t rate)
{
    switch(rate)
    {
        case 9600:
            return B9600;
        case 115200:
            return B115200;
        case 256000: //can't support B256000 ?
        default:
            return B115200;
    }
}

/*设置串口参数*/
int init_tty(int fd, const speed_t rate)
{
    struct termios  tty;

    speed_t baudrate = Convert2StandBaudRate(rate);

    bzero(&tty, sizeof(tty));

    if(tcgetattr(fd, &tty) != 0)
    {
        perror("tcgetatttr");
        return -1;
    }

    cfmakeraw(&tty);//设置终端属性，激活选项

    cfsetispeed(&tty, baudrate);//输入波特率
    cfsetospeed(&tty, baudrate);//输出波特率

    tty.c_cflag |= (CLOCAL | CREAD);//本地连接和接收使能

    tty.c_cflag &= ~CSIZE;//清空数据位
    tty.c_cflag |= CS8;//数据位为8位
    tty.c_cflag &= ~PARENB;//无奇偶校验
    tty.c_cflag &= ~CSTOPB;//一位停止位
    tty.c_cflag &= ~CRTSCTS;//no hardware flow control.

    //fetch bytes once they are availble.
    tty.c_cc[VTIME] = 1;//0.1second=100ms
    tty.c_cc[VMIN] = 1; //at least one byte


    /* set for non-canonical mode*/
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_oflag &= ~OPOST;
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);


    if(tcsetattr(fd, TCSANOW, &tty) != 0)//激活串口设置
        return -1;

    printf("set tty ok. baudRate = %d \n", rate);
    return 0;
}



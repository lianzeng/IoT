//
// Created by lianzeng on 18-5-24.
//

#include <sys/timerfd.h>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <cstdio>
#include "canBuffer.hpp"

extern int createTimerFd(const int period,const int nsec);

struct ElmCmdRsp
{
    const char* cmd;
    const char* resp;

};

bool sendCmdAndCheckRsp(const int timerfd,const int fd, const ElmCmdRsp& cmdRsp)
{
    //send cmd on first timer expire, check rsp on second timer expire
    static CanBusBuffer canData; //use static to avoid stackoverflow
    int timerExpireCount = 0;
    const int MAXLOOP = 20;//assume the obd data can be received complete within 20 select().

    canData.clear();
    for(int i = 0; i < MAXLOOP; i++)
    {
        fd_set readset;
        int maxfdp1 = fd > timerfd ? (fd + 1) : (timerfd + 1);
        int result;
        do {
            FD_ZERO(&readset);
            FD_SET(fd, &readset);
            FD_SET(timerfd, &readset);
            result = select(maxfdp1, &readset, NULL, NULL, NULL);
        } while (result == -1 && errno == EINTR);

        if (result > 0)
        {
            if (FD_ISSET(timerfd, &readset))
            {
                timerExpireCount++;
                uint64_t dummy;
                read(timerfd, &dummy, sizeof(dummy));
                if(timerExpireCount >=2 ) break;

                if(::write(fd, cmdRsp.cmd,  strlen(cmdRsp.cmd)) != strlen(cmdRsp.cmd))
                    printf("send cmd error!\n");

            }
            if(FD_ISSET(fd, &readset))
            {
                canData.receiveData(fd);
            }
        }
    }

    //check resp from canData
    if(canData.data().find(cmdRsp.resp) == std::string::npos)
    {
        printf("check fail,receivd: %s,  expect:%s \n", canData.data().data(), cmdRsp.resp);
        return false;
    }
    else
    {
        printf("check ok, cmd:%s , received: %s \n", cmdRsp.cmd, canData.data().data());
        return true;
    }

}

bool initObdDevice(const int ttyfd)
{
//ELM327cmd:  ATI(info),ATZ(reset),ATE0(must,echo off),ATM1(memory on),ATL0(linefeed),ATS0(NoSpace),
// ATH1(must,canId),ATAT1(must,adaptive timing),ATSP0(must,automatic search protocol),0100(determin protocol),
// ATDP(describe protocol),ATDPN,010C,010D,..

    //the following cmd must keep order to send and check.
    const ElmCmdRsp cmdRsp[] = {{"ATI\r","ELM327"},
                          {"ATE0\r","OK"},
                          {"ATS0\r","OK"},
                          {"ATH1\r","OK"},
                          {"ATAT1\r","OK"},
                          {"ATSP0\r","OK"},
                          {"0100\r","8"},//Canid=18DAF110 or 7E8,if resp="NO DATA", then car is off.
                          {"ATDP\r","ISO"},//AUTO, ISO 15765-4 (CAN 11/250)
                          {"ATDPN\r","A"},//A8=ISO 15765-4 (CAN 11/250),A6=ISO 15765-4 (CAN 11/500)
                          {"010C\r","0C"}//7E807410C0C80000000,databyte2=PID,0C means RPM
                         };
    //send cmd every 50ms,check the resp
    int timerfd = createTimerFd(1,0);//interval=1s to wait resp

    for(int i= 0; i< sizeof(cmdRsp)/sizeof(cmdRsp[0]); i++)
    {
        if(!sendCmdAndCheckRsp(timerfd,ttyfd,cmdRsp[i]))
        {
            printf("initial OBD fail at: %s \n",cmdRsp[i].cmd);
            return false;
        }
    }
    printf("initial OBD succsess \n");
    return true;
}

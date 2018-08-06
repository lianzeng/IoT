//
// Created by lianzeng on 18-5-24.
//

#include <sys/timerfd.h>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <cstdio>
#include <sys/time.h>
#include "canBuffer.hpp"
#include "ISO15765Parse.hpp"
#include "shareMemData.h"

extern int createTimerFd(const int period,const int nsec);
extern ObdDataInfo g_obdDataFetched;


struct ElmCmdRsp
{
    const char* cmd;
    const char* resp;

};

typedef void (*ParseFun)(const char*);

struct ISO15765
{
    const char* cmd;
    ParseFun    pfun;
    int         payloadMinLen;//exclude canID, this is determined by the caculation formular
};

std::string sendCmdAndWaitRsp(const int timerfd,const int fd, const char* cmd)
{
    //send cmd on first timer expire, check rsp on second timer expire
    //wait 1st timer expire---send cmd---receive data going---2nd timer expire---return data;

    static CanBusBuffer canData; //use static to avoid stackoverflow
    canData.clear();

    int timerExpireCount = 0;
    const int MAXLOOP = 30;//assume the obd data can be received complete within 30 select(), but not exceed timerfd;

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

                if(::write(fd, cmd,  strlen(cmd)) != strlen(cmd))//send cmd at first expire
                {
                    printf("send cmd error!\n");
                }
                //else
                  //  printf("\n send: %s \n", cmd); //debug only

            }
            if(FD_ISSET(fd, &readset))
            {
                canData.receiveData(fd);//receive data between first timer expire and second timer expire
            }
        }
    }

    return canData.data();
}

bool initObdDevice(const int ttyfd) //ISO15765-4
{
//ELM327cmd:  ATI(info),ATZ(reset),ATE0(must,echo off),ATM1(memory on),ATL0(linefeed),ATS0(NoSpace),
// ATH1(must,canId),ATAT1(must,adaptive timing),ATSP0(must,automatic search protocol),0100(determin protocol),
// ATDP(describe protocol),ATDPN,010C,010D,..

    //the following cmd is for ISO15765-4, must keep order to send and check.
    const ElmCmdRsp cmdRsp[] = {{"ATI\r","ELM327"},
                          {"ATE0\r","OK"},
                          {"ATS0\r","OK"},
                          {"ATH1\r","OK"},
                          {"ATAT1\r","OK"},
                          {"ATSP0\r","OK"}, //automatic search and detect obd protocol, but can't detect j1939
                          {"0100\r","SEARCH"},
                          {"0100\r","8"},//for ISO15765-4,Canid=18DAF110 or 7E8,if resp="unable to connect", then car is off
                          {"ATDP\r","ISO"},//AUTO, ISO 15765-4 (CAN 11/250)
                          {"ATDPN\r","A"},//A8=ISO 15765-4 (CAN 11/250),A6=ISO 15765-4 (CAN 11/500)
                          {"010C\r","0C"}//7E807410C0C80000000,databyte2=PID,0C means RPM
                         };
    //send cmd every 50ms,check the resp
    int timerfd = createTimerFd(2,0);//interval=2s to wait cmd resp
    int searchTimerFd = createTimerFd(10,0);//interval=10s to wait cmd resp

    for(int i= 0; i< sizeof(cmdRsp)/sizeof(cmdRsp[0]); i++)
    {
        int tfd = (i == 6) ? searchTimerFd : timerfd; //search need longer timer, to refactor
        std::string resp = sendCmdAndWaitRsp(tfd,ttyfd,cmdRsp[i].cmd);

        if(resp.find(cmdRsp[i].resp) == std::string::npos)
        {
            printf("initial OBD fail at cmd: %s \n",cmdRsp[i].cmd);
            printf("receivd: %s,  expect:%s \n", resp.data(), cmdRsp[i].resp);
            return false;
        }
        else
        {
            printf("check ok, cmd:%s , received: %s \n", cmdRsp[i].cmd, resp.data());
        }
    }
    printf("initial OBD succsess \n");
    return true;
}


void beginMonitorAllMsg(const int ttyfd)
{
    //send "ATMA\r" to begin monitor  all msg, this cmd itself not have resp
    const char* monitorCmd = "ATMA\r"; //easy to cause ELM327 buffer full
    if(::write(ttyfd, monitorCmd,  strlen(monitorCmd)) != strlen(monitorCmd))//send cmd at first expire
        printf("send ATMA error!\n");
}

bool initiCarDevice_J1939(const int ttyfd)
{
//ELM327cmd:  ATI(info),ATZ(reset),ATE0(must,echo off),ATL0(linefeed),ATS0(NoSpace),
// ATH1(must,canId),ATAT1(must,adaptive timing),ATSPA(must,j1939 protocol),
// ATDP(describe protocol),ATDPN,ATD1(open DLC=8),ATMA(start monitor)..

    //the following cmd is for ISO15765-4, must keep order to send and check.
    const ElmCmdRsp cmdRsp[] = {//{"ATI\r","ELM327"},
                                {"ATE0\r","OK"},
                                {"ATS0\r","OK"},
                                {"ATH1\r","OK"},
                                {"ATAT1\r","OK"},
                                {"ATSPA\r","OK"},//set to j1939
                                {"ATDP\r","1939"},//SAE J1939(can29/250)
                                {"ATD1\r","OK"},//open DLC
                                {"ATJHF0\r","OK"} //keep orignal PGN format
    };
    //send cmd every 50ms,check the resp
    int timerfd = createTimerFd(3,0);//interval=3s to wait cmd resp


    for(int i= 0; i< sizeof(cmdRsp)/sizeof(cmdRsp[0]); i++)
    {

        std::string resp = sendCmdAndWaitRsp(timerfd,ttyfd,cmdRsp[i].cmd);

        if(resp.find(cmdRsp[i].resp) == std::string::npos)
        {
            printf("initial OBD fail at cmd: %s \n",cmdRsp[i].cmd);
            printf("receivd: %s,  expect:%s \n", resp.data(), cmdRsp[i].resp);
            return false;
        }
        else
        {
            printf("check ok, cmd:%s , received: %s \n", cmdRsp[i].cmd, resp.data());
        }
    }


    printf("initial OBD succsess \n");
    return true;
}

time_t getUtcTime()
{
    struct timeval tv;

    if (0 != gettimeofday(&tv, NULL))
    {
        return -1;
    }

    return tv.tv_sec;
}


extern ObdDataInfo g_obdDataFetched;
extern int g_fdIpc;
extern void sendIpcData(const int ipcfd, ObdDataInfo* obdData);
extern void stub_temperature_sendIpcData(const int ipcfd);
extern void stub_VehicleValue( ObdDataInfo* obdData);
extern void stub_gps_sendIpcData(const int ipcfd);
extern void stub_pm25_sendIpcData(const int ipcfd);
extern void stub_humidity_sendIpcData(const int ipcfd);



void fetchObdData(const int ttyfd, const int timerfd)
{
//if obd-device is power off, this function will send cmd error, but won't crash.

    memset(&g_obdDataFetched, 0, sizeof(g_obdDataFetched));

    //note: the cmd need uppercase, 010C, not 010c
    const ISO15765 itemsList[] = {{"010C\r",fomular1_ISO15765,10}, //engine speed(RPM), emerge brake
                                  {"010D\r",fomular2_ISO15765,8},  //vechile speed, running state
                                  {"0105\r",fomular3_ISO15765,8}, //engCoolTemp
                                  {"0131\r",fomular4_ISO15765,10}, //distance after clean fault
                                  {"011F\r",fomular5_ISO15765,10}, //eng run time
                                  {"03\r",  fomular6_ISO15765,6},  //DTC fault num and detail , refer to ELM327-Page36
                                  {"0104\r",fomular7_ISO15765,8},   //eng load
                                  {"0121\r",fomular8_ISO15765,10},  //distance with fault happen
                                  {"012F\r",fomular9_ISO15765,8}, //fuel Level Input,percent of leftFuel/capacity
                                  {"014D\r",fomular10_ISO15765,10}, //engRunTimeWithFault
                                  {"014E\r",fomular11_ISO15765,10}, //engRunTimeWithoutFault
                                 };



    static int idx = 0;
    printf("\n %d: ", idx++);//just for view

    for(int i= 0; i< sizeof(itemsList)/sizeof(itemsList[0]); i++)
    {
        const char* payload = NULL;
        std::string resp = sendCmdAndWaitRsp(timerfd,ttyfd,itemsList[i].cmd);
        if(payload = checkISO15765DataFormat(resp,itemsList[i].payloadMinLen)) {
            if (itemsList[i].pfun)
                itemsList[i].pfun(payload);
        }
    }

    g_obdDataFetched.timestamp = getUtcTime();
    g_obdDataFetched.totalDistance = g_obdDataFetched.distanceWithFault + g_obdDataFetched.distanceWithoutFault;
    g_obdDataFetched.totalVehicleHours = g_obdDataFetched.engRunTimeWithFault + g_obdDataFetched.engRunTimeWithoutFault;

    //stub_VehicleValue(&g_obdDataFetched);
    sendIpcData(g_fdIpc, &g_obdDataFetched);
    //stub_temperature_sendIpcData(g_fdIpc);//just for integration test
    //stub_gps_sendIpcData(g_fdIpc);//just for integration test
    //stub_pm25_sendIpcData(g_fdIpc);
    //stub_humidity_sendIpcData(g_fdIpc);

}


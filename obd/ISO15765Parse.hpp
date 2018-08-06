//
// Created by lianzeng on 18-5-24.
//

#ifndef OBD_ISO15765PARSE_HPP
#define OBD_ISO15765PARSE_HPP

#include <string>
#include <cassert>
#include <cstdio>
#include "reportJ1939.hpp"

extern int byte2hex(char H, char L);
extern ObdDataInfo g_obdDataFetched;

static inline const char* checkISO15765DataFormat(const std::string& data, const int minLen)
{
    const char* extCanId = "18DAF110";
    const char* stdCanId = "7E8";
    size_t head = 0;
    const char* payload = NULL;
    if((head=data.find(extCanId)) != std::string::npos)
    {
        payload =  (data.size() >= (8+minLen)) ? &data[head+8] : NULL;//8=headerSize
    }
    else if((head=data.find(stdCanId)) != std::string::npos)
    {
        payload = (data.size() >= (3+minLen)) ? &data[head+3] : NULL;//3=headerSize
    }
    else
    {
        payload =  NULL;
    }

    if(payload == NULL)
        printf(" invalid data: %s \n", data.data());

    return payload;
}

//Note:byte from 0, has no space
int byteIndex(const char* d, int i)
{
    assert(0<= i && i < 8);
    return byte2hex(d[i*2],d[i*2+1]);
}

static inline void fomular1_ISO15765(const char* payload) //engine speed, RPM
{
    if(payload[4]=='0' && payload[5]=='C') //pid="0C", RPM,0741xxFFFFFFFFFF
    {
        int b3 = byteIndex(payload,3);
        int b4 = byteIndex(payload,4);
        float rpm =  (float)((b3<<8)+b4)/4.0;
        printf("rpm=%.3f  ", rpm);
        g_obdDataFetched.engSpeed = rpm;

        static const float threshold = 300;//rpm
        static float previousRpm = 0.0;
        if((rpm - previousRpm) > threshold || (previousRpm - rpm) > threshold)
        {
            g_obdDataFetched.emergeBrake = 1;
            printf("emerge brake happen,  ");
        }
        previousRpm = rpm;
    }
}

static inline void fomular2_ISO15765(const char* payload) //vehicle speed
{
    if(payload[4]=='0' && payload[5]=='D')
    {
        int vspeed =  byteIndex(payload,3);
        printf("vspeed=%d km/h, ",vspeed);
        g_obdDataFetched.vehicleSpeed = vspeed;
        strcpy(g_obdDataFetched.state , (vspeed > 0) ? "running" : "stopped");
    }
}

static inline void fomular3_ISO15765(const char* payload) //engCoolTemp
{
    if(payload[4]=='0' && payload[5]=='5')
    {
        int engCoolT =  byteIndex(payload,3)- 40;
        printf("engCoolT=%d C, ",engCoolT);
        g_obdDataFetched.engCoolanTemperature = engCoolT;
    }
}

static inline void fomular4_ISO15765(const char* payload) //distance after clean fault
{
    if(payload[4]=='3' && payload[5]=='1')
    {
        int b3 =  byteIndex(payload,3);
        int b4 =  byteIndex(payload,4);
        int dist =  (b3<<8)+b4;
        g_obdDataFetched.distanceWithoutFault = dist;
        printf("distanceWithoutFault=%d Km, ",dist);
    }
}

static inline void fomular5_ISO15765(const char* payload) //eng run time
{
    if(payload[4]=='1' && payload[5]=='F')
    {
        int b3 =  byteIndex(payload,3);
        int b4 =  byteIndex(payload,4);
        int drivetime =  (b3<<8)+b4;
        g_obdDataFetched.engRunTime = ((double)drivetime)/3600;
        printf("eng run time=%d sec, ",drivetime);
    }
}

static inline void fomular6_ISO15765(const char* payload) //DTC fault num
{
    /*
    no dtc:  18DAF110024300
    P1012:   18DAF1100443011012
P1012 p1013: 18DAF11006430210121013
    */

    int faultNum = byteIndex(payload, 2); //if actual dtc = 0, byteIndex() will report error: "invalid ascii ,not a number"; but it doesn't matter;

    faultNum = (faultNum > MAX_FAULT_NUM_REPORT) ? MAX_FAULT_NUM_REPORT : faultNum;

    if(faultNum > 0 && (strlen(payload) >= (faultNum*4+6)))//TODO: when dtc >2,  parse fault detail wrong.
    {
        int len = 0;
        for(int i = 0; i < faultNum; i++) {
            len += sprintf(g_obdDataFetched.falutDetail+len, "P");
            strncpy(g_obdDataFetched.falutDetail+len,payload+4*i+6,4);
            len += 4;
            len += sprintf(g_obdDataFetched.falutDetail+len," ");
        }
    }

    g_obdDataFetched.faultNum = faultNum;

    printf("DTC num=%d , detail:%s ,",faultNum, g_obdDataFetched.falutDetail);//DTC num=2 , detail:P1012 P1013
}

static inline void fomular7_ISO15765(const char* payload) //engine load
{
    if(payload[4]=='0' && payload[5]=='4')
    {
        float b3 =  byteIndex(payload,3);
        float load = b3*100/255;
        g_obdDataFetched.engLoad = load;
        printf("engLoad=%.2f%% , ", load);
    }
}

static inline void fomular8_ISO15765(const char* payload) //distance with fault happen, but not cleared
{
    if(payload[4]=='2' && payload[5]=='1')
    {
        int b3 =  byteIndex(payload,3);
        int b4 =  byteIndex(payload,4);
        int dist =  (b3<<8)+b4;
        g_obdDataFetched.distanceWithFault = dist;
        printf("distanceWithFault=%d Km, ",dist);
    }
}

static inline void fomular9_ISO15765(const char* payload) //percent of leftFuel/capacity
{
    if(payload[4]=='2' && payload[5]=='F')
    {
        float b3 =  byteIndex(payload,3);
        float fuelLevelInput = b3*100/255;
        g_obdDataFetched.fuelLevelInput = fuelLevelInput;
        printf("fuelLevelInput=%.2f%% , ", fuelLevelInput);
    }
}

static inline void fomular10_ISO15765(const char* payload) //engRunTimeWithFault
{

    if(payload[4]=='4' && payload[5]=='D')
    {
        int b3 =  byteIndex(payload,3);
        int b4 =  byteIndex(payload,4);
        int runtime =  (b3<<8)+b4;
        g_obdDataFetched.engRunTimeWithFault = ((float)runtime)/60;
        printf("engRunTimeWithFault=%d minutes, ",runtime);
    }
}

static inline void fomular11_ISO15765(const char* payload) //engRunTimeWithoutFault
{

    if(payload[4]=='4' && payload[5]=='E')
    {
        int b3 =  byteIndex(payload,3);
        int b4 =  byteIndex(payload,4);
        int runtime =  (b3<<8)+b4;
        g_obdDataFetched.engRunTimeWithoutFault = ((float)runtime)/60;
        printf("engRunTimeWithoutFault=%d minutes, ",runtime);
    }
}

#endif //OBD_ISO15765PARSE_HPP

//
// Created by zengliang on 18-7-17.
//

#include <cstdio>
#include <ctime>
#include <cstring>
#include "reportJ1939.hpp"


#define UNIX_IPC_NAME  "/var/run/lwm2m.sock"

extern time_t getUtcTime();
extern void send_Dgram(const int ipcfd, const char* buffer);



void stub_VehicleValue( ObdDataInfo* obdData)
{
    static  int vspeed = 60;
    static int vspeedDelta = 1;
    if(vspeed >= 240)
        vspeedDelta = -1;
    else if(vspeed <= 0)
        vspeedDelta = 1;

    vspeed += vspeedDelta;

    static float lastTime_s = getUtcTime();
    float currTime_s = getUtcTime();
    double elapsedTime = currTime_s - lastTime_s;
    lastTime_s = currTime_s;

    strcpy(obdData->state , (vspeed > 0) ? "running" : "stopped");
    obdData->vehicleSpeed = vspeed;//km/h

    static float engSpeed = 800;
    if(engSpeed < 0.01) engSpeed = 800;
    engSpeed -= 1;

    obdData->engSpeed = engSpeed;    //rpm
    obdData->engCoolanTemperature = 30; //C.
    obdData->engLoad = 75;

    static float fuelLevelInput = 100.0;
    if(fuelLevelInput <= 0.01) fuelLevelInput = 100.0;
    fuelLevelInput -= 1;
    obdData->fuelLevelInput = fuelLevelInput;

    //obdData->engFuelRate = 10.9;//  L/100km
    obdData->faultNum = 1;
    strcpy(obdData->falutDetail,"P1012 ");
    //obdData->engTripFuelUsed = 20;
    static double totalVehicleHours = 0.0;
    totalVehicleHours += (double)5.56e-4;
    obdData->totalVehicleHours = totalVehicleHours;
    static double distance = 0.0;
    distance += (double)vspeed*5.56e-4;//(elapsedTime/3600);
    obdData->totalDistance = distance;
    obdData->batteryVoltage = 13;
    obdData->emergeBrake = 1;
    obdData->timestamp = getUtcTime();


}

void stub_temperature_sendIpcData(const int ipcfd)
{
    //const char* buffer = "{\"3303\":{\"0\":{\"5700\":\"36.5\",\"5800\":\"80\",\"5524\":\"300\"}, \"4\":{\"5700\":\"-18.5\",\"5800\":\"5\"}}}";

#if 1
    static const char* output = "{\"10255\":{\"3\":{\"5519\":\"-30.0\",\"5520\":\"60.0\",\"5701\":\"cel\",\"5800\":\"89.7\",\"5906\":\"MOT-U202\",\"5907\":\"FE:97:18:09:14:36\",\"6021\":\"1533104430\",\"5524\":\"300\",\"5700\":\"25.13\"},\"4\":{\"5519\":\"-30.0\",\"5520\":\"60.0\",\"5701\":\"cel\",\"5800\":\"99.0\",\"5906\":\"MOT-U202\",\"5907\":\"FE:97:18:09:31:63\",\"6021\":\"1533104430\",\"5524\":\"300\",\"5700\":\"25.29\"},\"0\":{\"5519\":\"-30.0\",\"5520\":\"60.0\",\"5701\":\"cel\",\"5800\":\"102.1\",\"5906\":\"MOT-U202\",\"5907\":\"FE:97:18:09:26:15\",\"6021\":\"1533104430\",\"5524\":\"300\",\"5700\":\"25.28\"}}}";
#else
    static  float tempture_value = 0.0;
    static int delta = 0.5;
    if(tempture_value >= 60)
        delta = -0.5;
    else if(tempture_value <= -30)
        delta = 0.5;

    tempture_value += delta;

    static float tempture_battery = 100.0;
    if(tempture_battery > 0.1) tempture_battery -= 0.01;
    else tempture_battery = 0.0;


    static char output[200];
    int len = 0;
    len += sprintf(output+len,"{\"10255\":{\"3\":{");//objId=3303,instId=3

    len += sprintf(output+len,"\"5700\":");
    len += sprintf(output+len,"\"%.2f\"",tempture_value);
    len += sprintf(output+len,",\"5800\":");
    len += sprintf(output+len,"\"%.2f\"",tempture_battery);
    len += sprintf(output+len,",\"5524\":\"300\"");//sample interval=300

    len += sprintf(output+len,",\"5519\":\"-30\"");
    len += sprintf(output+len,",\"5520\":\"100\"");
    len += sprintf(output+len,",\"5601\":\"-20\"");
    len += sprintf(output+len,",\"5602\":\"30\"");
    len += sprintf(output+len,",\"5701\":\"C\"");

    len += sprintf(output+len,",\"6021\":");
    len += sprintf(output+len,"\"%ld\"",getUtcTime());

    len += sprintf(output+len,",\"5906\":");
    len += sprintf(output+len,"\"%s\"","MOT-U202");

    len += sprintf(output+len,",\"5907\":");
    len += sprintf(output+len,"\"%s\"","FE:91:18:09:31:63");

    len += sprintf(output+len,",\"5527\":");
    len += sprintf(output+len,"\"%s\"","25.00,26.00,27.00");


    len += sprintf(output+len, "}}}");
    output[len] = 0;

#endif
    printf("\n JSON: %s \n", output);


    send_Dgram(ipcfd, output);

}


void stub_gps_sendIpcData(const int ipcfd)
{
    static float latitude = 30.163539;
    static float longitude = 120.09172;
    static float altitude = 40.00;
    static float radius = 0;
    static float speed = 80;

    static char output[200];
    int len = 0;
    len += sprintf(output+len,"{\"6\":{\"0\":{");//objId=6,instId=0

    len += sprintf(output+len,"\"0\":");
    len += sprintf(output+len,"\"%.6f\"",latitude);
    len += sprintf(output+len,",\"1\":");
    len += sprintf(output+len,"\"%.5f\"",longitude);
    len += sprintf(output+len,",\"2\":");
    len += sprintf(output+len,"\"%.2f\"",altitude);
    len += sprintf(output+len,",\"3\":");
    len += sprintf(output+len,"\"%.2f\"",radius);

    len += sprintf(output+len,",\"5\":");
    len += sprintf(output+len,"\"%ld\"",getUtcTime());
    len += sprintf(output+len,",\"6\":");
    len += sprintf(output+len,"\"%.2f\"",speed);

    len += sprintf(output+len, "}}}");
    output[len] = 0;
    printf("\n JSON: %s \n", output);

    send_Dgram(ipcfd, output);
}


void stub_pm25_sendIpcData(const int ipcfd)
{
    const char* id = "PM25_123456789" ;
    static int value = 10;
    value += 1;

    static char output[200];
    int len = 0;
    len += sprintf(output+len,"{\"10256\":{\"0\":{");//objId=6,instId=0

    len += sprintf(output+len,"\"0\":");
    len += sprintf(output+len,"\"%s\"",id);
    len += sprintf(output+len,",\"1\":");
    len += sprintf(output+len,"\"%d\"",value);

    len += sprintf(output+len, "}}}");
    output[len] = 0;
    printf("\n JSON: %s \n", output);

    send_Dgram(ipcfd, output);
}

void stub_humidity_sendIpcData(const int ipcfd)
{
    const char* id = "HUMIDITY_123456" ;
    static int value = 100;
    value += 1;

    static char output[200];
    int len = 0;
    len += sprintf(output+len,"{\"10257\":{\"0\":{");//objId=6,instId=0

    len += sprintf(output+len,"\"0\":");
    len += sprintf(output+len,"\"%s\"",id);
    len += sprintf(output+len,",\"1\":");
    len += sprintf(output+len,"\"%d\"",value);
    len += sprintf(output+len,",\"2\":");
    len += sprintf(output+len,"\"%.2f\"",99.99);

    len += sprintf(output+len, "}}}");
    output[len] = 0;
    printf("\n JSON: %s \n", output);

    send_Dgram(ipcfd, output);
}
//
// Created by impact on 18-7-4.
//

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cerrno>
#include "reportJ1939.hpp"


#define UNIX_IPC_NAME  "/var/run/lwm2m.sock"      //filename must be absolute path, but not already exist
#define CLIENT_NAME "obdclient"



char* convertDataToJSON(ObdDataInfo* obdData)
{

    //JSON: {"10244":{"0":{"0":"running","1":"160.00","2":"800.00","3":"1531210007","4":"1 : P1012 ","5":"75","6":"0.60","7":"30"}}}

    const int VEHICLE_OBJECT_ID = 10244;


    static char output[200];

    int len = 0;
    len += sprintf(output+len,"{");
    len += sprintf(output+len,"\"%d\":",VEHICLE_OBJECT_ID);//objId

    len += sprintf(output+len, "{\"0\":{"); //instanceid

    len += sprintf(output+len, "\"0\":"); //resourceId = 0
    //const char* state = (obdData->vehicleSpeed > 0) ? "running" : "stopped";
    len += sprintf(output+len, "\"%s\"", obdData->state);

    len += sprintf(output+len,",\"1\":");
    len += sprintf(output+len,"\"%.2f\"",obdData->vehicleSpeed);

    len += sprintf(output+len,",\"2\":");
    len += sprintf(output+len,"\"%.2f\"",obdData->engSpeed);

    len += sprintf(output+len,",\"3\":");
    len += sprintf(output+len,"\"%ld\"",obdData->timestamp);

    len += sprintf(output+len,",\"4\":");
    len += sprintf(output+len,"\"%d : %s\"",obdData->faultNum,obdData->falutDetail);

    len += sprintf(output+len,",\"5\":");
    len += sprintf(output+len,"\"%d\"",obdData->engLoad);

    len += sprintf(output+len,",\"6\":");
    len += sprintf(output+len,"\"%.2f\"",obdData->fuelLevelInput);

    len += sprintf(output+len,",\"7\":");
    len += sprintf(output+len,"\"%d\"",obdData->engCoolanTemperature);

    len += sprintf(output+len,",\"8\":");
    len += sprintf(output+len,"\"%.4f\"",obdData->totalVehicleHours);

    len += sprintf(output+len,",\"9\":");
    len += sprintf(output+len,"\"%.3f\"",obdData->totalDistance);

    len += sprintf(output+len,",\"10\":");
    len += sprintf(output+len,"\"%.2f\"",obdData->batteryVoltage);//not found for ISO15765

    len += sprintf(output+len,",\"11\":");
    len += sprintf(output+len,"\"%d\"",obdData->emergeBrake);//emerge brake

    len += sprintf(output+len, "}}}");

    output[len] = 0;

    printf("\n JSON: %s \n", output);
    return output;
}

int createUnixSocket()
{
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        perror("create unix socket");
        exit(EXIT_FAILURE);
    }

    unlink(CLIENT_NAME);
    struct sockaddr_un client;
    memset(&client, 0, sizeof(client));

    client.sun_family = AF_UNIX;
    strcpy(client.sun_path, CLIENT_NAME);

    if (bind(fd, (struct sockaddr *) &client, sizeof(client)))
    {
        perror("bind unix socket");
        exit(EXIT_FAILURE);
    }

    printf("create unix Socket ok, fd=%d : %s\n", fd, client.sun_path);
    return fd;
}


void send_Dgram(const int ipcfd, const char* buffer)
{
    struct sockaddr_un server;
    memset(&server, 0, sizeof(server));
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, UNIX_IPC_NAME);

    //even if lwm2mclient(udp server) not run, this program still ok.
    int numBytes = sendto(ipcfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server, sizeof(server));

    if(numBytes != strlen(buffer))
    {
        printf("sendto lwm2mclient error=%d : %s \n",errno,  strerror(errno));
    }
    else
        printf("sendto lwm2mclient ok.\n");
}

void sendIpcData(const int ipcfd, ObdDataInfo* obdData)
{
    //const char* buffer = "{\"10244\":{\"0\":{\"0\":\"running\",\"1\":\"800\",\"2\":\"300\",\"3\":\"123456789ABCD\"}}}";
    const char* buffer = convertDataToJSON(obdData);
    send_Dgram(ipcfd, buffer);

}





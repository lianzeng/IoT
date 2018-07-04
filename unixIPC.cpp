//
// Created by impact on 18-7-4.
//

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cerrno>

#define UNIX_IPC_NAME  "/var/lwm2mReport_ipc"      //filename must be absolute path, but not already exist
#define CLIENT_NAME "obdclient"


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


void sendIpcData(int fd)
{
    const char* buffer = "{objId:10244,instId:0,resNum:4,resValues:[0:1,1:100,2:800,3:1234567]}";


    struct sockaddr_un server;
    memset(&server, 0, sizeof(server));

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, UNIX_IPC_NAME);


    int numBytes = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *)&server, sizeof(server));

    if(numBytes != strlen(buffer))
    {
           printf("sendto lwm2mclient error=%d : %s \n",errno,  strerror(errno));
    }
    else
        printf("sendto lwm2mclient ok.\n");

}
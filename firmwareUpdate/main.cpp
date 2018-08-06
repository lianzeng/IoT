#include <iostream>
#include <sys/select.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/un.h>

/*

 wait to receive cmd from lwm2mclient process, then excute :
 1)when receive download cmd with url, begin to call script to download package from given url, after download finished or fail,feedback state to lwm2mclient;
 2)when receive firmware update cmd , begin to update firmware ,  feedback state to lwm2mclient;

 the new package will be download into the same dir as currernt program.

 */



extern int createUnixSocket();

extern char* receive_Dgram(int fd, struct sockaddr_un* peer);
extern void send_Dgram(const int ipcfd, const char* buffer);
extern void send_Dgram(const int ipcfd, const struct sockaddr_un* peer, const char* buffer);



enum Cmd{DOWNLOAD = 0, UPDATE = 1};

const char* g_script[] =
{
        "/opt/lwm2m/firmwareDownload.sh  ",
        "/opt/lwm2m/firmwareUpgrade.sh  ",

};

int callScript(const char *scriptPath)
{
    int ret = -1;

    if(scriptPath == NULL) return ret;


    fprintf(stdout, "excute script: \" %s \" ", scriptPath);

    int status = system(scriptPath); //use script absolute path instead of relative path, system() will block until script finished

    // status is not the actual return status
    // first check if there were errors reported by system()
    if(WIFEXITED(status))
    {
        ret = WEXITSTATUS(status); // 0 = ok, other value = fail
        printf("\n script excuted normally \n");
        printf("\n scipt exit status: %d \n", ret);

    }
    else
    {
        printf("error:script not excuted \n");
    }

    return ret;
}



bool containURL(const char *cmd) //
{
    static const int minLen = strlen("http://x.x.x.x");
    if(strlen(cmd) > minLen)
    {
        std::string str(cmd);
        if(str.find("http") == 0) //cmd start with http or https, support coap and coaps in future
        {
            return true;
        }
    }
    return  false;
}

bool isUpdateCmd(const char* cmd)//msg format: update
{
    std::string str(cmd);
    if(str.find("UPDATE") == 0)
        return true;
    else
        return false;
}

const char*  processCmd(const char* cmd)
{


    int ret = -1;

    if(containURL(cmd))
    {

        std::string url(cmd); //http://52.80.95.56:8038/edgeTest.txt
        std::string script( g_script[DOWNLOAD]);
        script += url;
        fprintf(stdout, "script:%s \n", script.data());

        ret = callScript(script.data());

        return  (ret == 0) ? "DL_OK" : "DL_FAIL";
    }
    else if(isUpdateCmd(cmd))
    {
        ret = callScript(g_script[UPDATE]);
        return (ret == 0) ? "UP_OK" : "UP_FAIL";
    }
    else {
        return "Unknow cmd";
    }
}

int main() {

    int fd = createUnixSocket();
    const char* msg = NULL;
    //static char result[16];

    struct sockaddr_un peer;
    memset(&peer, 0, sizeof(peer));

    while(true)
    {
        //memset(result, 0, sizeof(result));
        msg = receive_Dgram(fd, &peer);
        //msg = "HTTP_CMD:http://52.80.95.56:8038/edgeTest.txt";
        if(msg)
        {
            fprintf(stdout, "receivd msg:%s \n", msg);
            const char* result = processCmd(msg);
            send_Dgram(fd, &peer, result);

        }
    }


    fflush(stdout);
    return 0;
}
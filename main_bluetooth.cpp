#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <termio.h>
#include <cstring>

#include <sys/timerfd.h>
#include <cerrno>
#include <stdint.h>


int createTimerFd(const int sec,const int nsec)
{
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(timerfd < 0) {
        perror("timerfd_create failed");
        return -1;
    }

    struct itimerspec tmr;
    tmr.it_value.tv_sec = sec;
    tmr.it_value.tv_nsec = nsec;
    tmr.it_interval.tv_sec = sec;//periodically
    tmr.it_interval.tv_nsec = nsec;
    timerfd_settime(timerfd, 0, &tmr, NULL);

    //printf("faultfd = %d \n",timerfd);
    return timerfd;
}


void callScript()
{
    int status = system("./bluetooth_auto.sh"); //will block until script finished

    // status is not the actual return status
    // first check if there were errors reported by system()
    if(WIFEXITED(status))
    {
        printf("\n\n script excuted normally \n");
        printf("scipt exit status: %d \n", WEXITSTATUS(status));
    }
    else
    {
        printf("error:script not excuted \n");
    }
}


//this program shoud be a daemon process, never quit. if quit, the /dev/rfcomm0 will be removed .
//under x86 pc: sudo ./xxx; under arm: ./xxx
int main() {

    const int minute = 60;
    const int period = 1*minute;
    int timerfd = createTimerFd(period, 0);//10s for lab test, 1minutes for field test.

    callScript();

    //no matter obd is connected or not, periodically detect and try connect;
    //even if obd already connected, has no side effect;
    while(true){

        fd_set readset;
        int maxfdp1 = (timerfd + 1);
        int result;
        do {
            FD_ZERO(&readset);
            FD_SET(timerfd, &readset);
            result = select(maxfdp1, &readset, NULL, NULL, NULL);
        } while (result == -1 && errno == EINTR);

        if (result > 0) {
            if (FD_ISSET(timerfd, &readset)) {
                uint64_t dummy;
                read(timerfd, &dummy, sizeof(dummy));
                printf("\n\n detect obd every %d seconds \n\n", period);
                callScript();
            }
        }
        else
            printf("error in select \n");


    }


    return 0;
}
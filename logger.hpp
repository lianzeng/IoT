//
// Created by lianzeng on 18-4-25.
//

#ifndef OBD_LOGGER_HPP
#define OBD_LOGGER_HPP


#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include "reportJ1939.hpp"


class Logger
{
public:
    Logger(const char* logfile = NULL):fp(NULL),hasParasUpdated(false)
    {
       if(logfile != NULL)
       {
           const char* itemTitle = "VSpeed(km/h)         ESpeed(rpm)         ELoad(%)            \n";
           fp = fopen(logfile, "a+");
           fwrite(itemTitle,sizeof(char),strlen(itemTitle),fp);
       }
    }
    ~Logger()
    {
        fp = NULL;
        hasParasUpdated = false;
    }
    void report(const J1939Reports &report)//report to IMPACT or file
    {
        if(!fp || !hasParasUpdated) return;
        const int LENGTH = 100;//TODO:this buffer need extend if more items to report
        static char buffer[LENGTH] = {0};
        memset(buffer, ' ', LENGTH);
        buffer[LENGTH-2]='\n';
        buffer[LENGTH-1]='\0';
        snprintf(buffer+20*0,LENGTH-1,"%.3f",report.vehicleSpeed);
        snprintf(buffer+20*1,LENGTH-1,"%.3f",report.engSpeed);
        snprintf(buffer+20*2,LENGTH-1,"%d%%",report.engLoad);

        fwrite(buffer,sizeof(char),LENGTH,fp);
        flush();
        clearUpdateFlag();
    }
    void flush()
    {
        if(fp)
            fflush(fp);
    }
    void finish()
    {
        if(fp)
           fclose(fp);
    }
    void setUpdateFlag()
    {
        hasParasUpdated = true;
    }
    void clearUpdateFlag()
    {
        hasParasUpdated = false;
    }
private:
    FILE* fp;
    bool hasParasUpdated;
};


#endif //OBD_LOGGER_HPP

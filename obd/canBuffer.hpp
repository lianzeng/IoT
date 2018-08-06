//
// Created by lianzeng on 18-4-20.
//

#ifndef OBD_CANBUFFER_HPP
#define OBD_CANBUFFER_HPP

#include <cstring>
#include <string>
#include <unistd.h>


                          //currently only support 8bytes frame.
class CanBusBuffer
{
public:
    CanBusBuffer()
    {
       memset(buff, 0, sizeof(buff));
       pos = 0;
    }
    ~CanBusBuffer()
    {

    }
    void clear()
    {
        pos = 0;
        buff[LENGTH] = '\0';
    }
    void printReceiveData(const char* rec, const int len)
    {
        std::string temp(rec, len);
        printf("received: %s \n", temp.data());
    }
    void receiveData(const int fd)//append can bus data.
    {
        if(isFull())
        {
            //printf("\n warning:buffer is full\n");
            return;
        }
        int len = ::read(fd,buff+pos,LENGTH-pos);
        //printReceiveData(buff+pos, len);//only for debug, may confuse the output format.
        if(len > 0)
            pos += len;

    }

    bool isFull() const
    {
      return pos >= LENGTH;
    }

    bool empty() const
    {
        return pos == 0;
    }

    std::string data() const
    {
        return std::string(buff, pos);
    }


private:
    static const int LENGTH = 100;
    char buff[LENGTH+1];
    int  pos;//number of bytes in buffer
};


#endif //OBD_CANBUFFER_HPP

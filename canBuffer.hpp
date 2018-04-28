//
// Created by lianzeng on 18-4-20.
//

#ifndef OBD_CANBUFFER_HPP
#define OBD_CANBUFFER_HPP

#include <cstring>
#include <string>
#include <unistd.h>

#define CAN_FRAME_LEN 34  //TODO:34 is length of "18FEF100 8 FC FF FE 04 00 00 00 00",need modify for ELM327
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
    void receiveData(const int fd)//append can bus data.
    {
        if(isFull())
        {
            printf("\n warning:buffer is full\n");
            return;
        }
        int len = ::read(fd,buff+pos,LENGTH-pos);
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
    std::string extractCanFrame(const std::string& canId, int& startPos) const
    {
        if(startPos < 0 || startPos >= pos) return "";
        std::string allData = data();
        size_t matchPos = allData.find(canId,startPos);
        if(matchPos != std::string::npos && matchPos < pos)
        {
            startPos = (matchPos + CAN_FRAME_LEN);
            int validBytes = pos - matchPos;
            validBytes = (validBytes < CAN_FRAME_LEN) ? validBytes : CAN_FRAME_LEN;
            return std::string(buff+matchPos,validBytes);
        }
        else
            return "";
    }

private:
    static const int LENGTH = 1024;//need extend to 20k.
    char buff[LENGTH+1];
    int  pos;
};


#endif //OBD_CANBUFFER_HPP

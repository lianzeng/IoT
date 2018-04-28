//
// Created by lianzeng on 18-4-23.
//

#ifndef OBD_FORMULA1939_HPP
#define OBD_FORMULA1939_HPP

#include <string>

typedef void (*Formula)(const std::string &canFrame);

struct Formula1939
{
    std::string canId;
    Formula     formula;
};




#endif //OBD_FORMULA1939_HPP

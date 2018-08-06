# -*- coding: utf-8 -*-
"""
Created on Wed Apr 25 13:49:42 2018

convert XiAn data to CanPro file format
eg: foton.txt ---》 foton_canpro.list
@author: lianzeng
"""

def extractTime(line:str)->str:
    t = line.split()[-1].strip('ms')
    return '"'+ t + '"'

def convert2CanMsg(line:str)->str:
    data = line.split()[0:-1]
    canId = data[0]
    if len(canId) < 8:
        return '' #print(line)        
    rcanId = canId[6]+canId[7] + canId[4] + canId[5] + canId[2] + canId[3] + canId[0] + canId[1]
    padd = '00000000000000010'
    payload = ''.join(data[1:])
    padd2 = '000000'
    return '"' + rcanId + padd + payload + padd2 + '"'
    

def convert(line:str)->str:
    if len(line) < 34:
        return ''
    msg = convert2CanMsg(line)
    if not msg:
        return ''
    prefix = '    <tagSendUint iInterval='
    interval = extractTime(line) #"1"
    middle = ' iTimes="1" len="1" bIncreaseID="0" bIncreaseData="0" obj='
    newline = prefix + interval + middle + msg + ' />'
    return newline

if __name__ == "__main__" :
    tfile = open("foton_canpro.list",'w')
    tfile.write('<SendList m_dwCycles="1">'+'\n')
    
    with open("foton.txt") as ifile:#with open("IVECO.txt",encoding="utf8") as iflie:
        for line in ifile:
            newline = convert(line)
            if newline:
                tfile.write(newline + '\n')
            
    tfile.write('</SendList>')
    tfile.close()            
    

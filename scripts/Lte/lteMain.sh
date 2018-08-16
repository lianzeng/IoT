#!/bin/bash

#
# Copyright (c) 2018 Nokia
#
#  All rights reserved. 
#
#  Contributors:
#
#

# DO NOT modify exit code, script exit value used by other programe
# function "EXIT" must be at last line if call with "$*"

key="MOBILE-NET"
W_MEM="/usr/bin/wmem"
R_MEM="/usr/bin/rmem"
REL_FILE="/dev/ttyUSB2"

RESET_ADDR="0x8002002"
POWER_ADDR="0x8002008"
LED_ADDR="0x80000A2"
LED_ADDR2="0x80000A6"
PPP_IF="ppp0"
PPP_PEERS_DIR="/etc/ppp/peers"
PPP_SCRIPT_DIR="/etc/ppp/scripts"
PPP_TAG="EDGE_EC20_1-1.1"
PID_FILE="/var/run/$PPP_IF.pid"
#TODO switch 4G/3G/2G
#AT+QCFG="nwscanmode",1,1
PPP_DATA="/dev/ttyUSB3"

g_retV=false

function generatePppFiles(){
  mkdir -p  $PPP_SCRIPT_DIR
  isFileExist $PPP_PEERS_DIR/$PPP_TAG
  retV=$?
  if [ $retV -ne 0 ]; then
    ECHO "generate file:"$PPP_PEERS_DIR/$PPP_TAG
cat > $PPP_PEERS_DIR/$PPP_TAG <<EOF
921600
unit 0
logfile /var/log/kura-$PPP_TAG
debug
connect 'chat -v -f /etc/ppp/scripts/chat_$PPP_TAG'
disconnect 'chat -v -f /etc/ppp/scripts/disconnect_$PPP_TAG'
modem
lock
noauth
noipdefault
defaultroute
usepeerdns
noproxyarp
novj
novjccomp
nobsdcomp
nodeflate
nomagic
active-filter 'inbound'
persist
holdoff 1
maxfail 5
connect-delay 1000
EOF
  fi

  isFileExist $PPP_PEERS_DIR/$PPP_IF
  retV=$?
  if [ $retV -ne 0 ]; then
    ECHO "generate file:"$PPP_PEERS_DIR/$PPP_IF
    ln -sf $PPP_PEERS_DIR/$PPP_TAG  $PPP_PEERS_DIR/$PPP_IF
  fi

  isFileExist $PPP_SCRIPT_DIR"/chat_"$PPP_TAG
  retV=$?
  if [ $retV -ne 0 ]; then
    ECHO "generate file:" $PPP_SCRIPT_DIR"/chat_"$PPP_TAG
cat > $PPP_SCRIPT_DIR"/chat_"$PPP_TAG <<EOF
ABORT   "BUSY"
ABORT   "VOICE"
ABORT   "NO CARRIER"
ABORT   "NO DIALTONE"
ABORT   "NO DIAL TONE"
ABORT   "ERROR"
""  "+++ath"
OK  "AT"
OK  at+cgdcont=1,"IP","3gnet"
OK  "\d\d\d"
""  "atd*99***1#"
CONNECT "\c"
EOF
  fi

  isFileExist $PPP_SCRIPT_DIR"/disconnect_"$PPP_TAG
  retV=$?
  if [ $retV -ne 0 ]; then
    ECHO "generate file:" $PPP_SCRIPT_DIR"/disconnect_"$PPP_TAG
cat > $PPP_SCRIPT_DIR"/disconnect_"$PPP_TAG<<EOF
ABORT   "BUSY"
ABORT   "VOICE"
ABORT   "NO CARRIER"
ABORT   "NO DIALTONE"
ABORT   "NO DIAL TONE"
""  BREAK
""  "+++ATH"
EOF
  fi
}

function isFileExist(){
    if [ ! -e $1 ];then
        ECHO "file $1 not exist."
        return 1
    else
        ECHO "file $1 exist."
        return 0
    fi 
}

function isPidExist(){
    if [ ! -d /proc/$1 ];then
        ECHO "process $1 not exist."
        return 1
    else
        ECHO "process $1 exist."
        return 0
    fi
}

function killByPidAndWait(){
    isPidExist $1
    retV=$?
    if [ $retV -eq 0 ]; then
        kill $1
        ECHO "kill Pid $1 wait $2 second(s)"
        sleep $2
    fi
}

function killPidFromPidFile(){
    if [ ! -e $1 ];then
        ECHO "pid file:$1 not found!"
        return 1 
    fi
    pid=-1
    while read line; do 
        pid=$line;
        # ECHO "found pid:$pid"
        break
    done < $1
    if [ $pid -eq -1 ]; then
        ECHO "pid not found in pid file."
        return 1
    else
        ECHO "found pid:$pid for process:$2"
        killByPidAndWait $pid $3
        return 0
    fi
}


function isPowerOn(){
    if [ -c $REL_FILE ]; then
        ECHO "[status] on!"
        turnLEDOn
        EXIT 0 $*
    else
        ECHO "[status] off!"
        turnLEDOff
        EXIT 1 $*
    fi
}

function powerOn(){
    isPowerOn z
    retV=$?
    if [ $retV -eq 0 ]; then
        ECHO "already turned on."
        EXIT 0 $*
    fi

    $W_MEM $RESET_ADDR 0x5A 2  # pull up LTE reset pin
    $W_MEM $POWER_ADDR 0x01 2  # pull down LTE power pin
    usleep 200000              # Wait for about 200ms
    $W_MEM $POWER_ADDR 0x00 2  # pull up LTE power pin

    # Wait for LTE module ready for work or 60 seconds passed.
    bl=false
    count=0
    while [ $count -lt 60 ]
    do
        isPowerOn z
        retV=$?
        if [ $retV -gt 0 ]; then
            count=`expr $count + 3`
            ECHO "wait $count seconds ..."
            sleep 3
        else
            ECHO "turn on successfully.This also turn on GNSS module."
            turnLEDOn
            bl=true
            break
        fi
    done
    if [[ ${bl} == true ]]; then 
    	if [ $# -eq 0 ]; then 
    		exit 0; 
    	else 
    		return 0; 
    	fi
    fi
    ECHO "turn on time out."
    turnLEDred
    EXIT 1 $*
}

function powerOff(){
    isPowerOn z
    retV=$?
    if [ $retV -ne 1 ]; then
        $W_MEM $POWER_ADDR 0x01 2  # pull down LTE power pin
        usleep 800000              # Wait for about 800ms
        $W_MEM $POWER_ADDR 0x00 2  # pull up LTE power pin

        count=0
        while [ $count -lt 60 ]
        do
            isPowerOn z
            retV=$?
            if [ $retV -ne 1 ]; then
                count=`expr $count + 7`
                ECHO "wait $count seconds ..."
                sleep 7
            else
                ECHO "turn off successfully.This also turn off GNSS module."
                turnLEDOff
                EXIT 0 $*
            fi
        done
        ECHO "turn off time out."
        turnLEDred
        EXIT 1 $*   
    else
        ECHO "already turned off."
        EXIT 0 $*
    fi
}

function resetModule(){
    isPowerOn z
    retV=$?
    if [ $retV -eq 0 ]; then
        ECHO "reseting ..."
        /usr/bin/wmem 0x8002002 0xA5 2  # pull down LTE reset pin
        usleep 800000                   # Wait for about 800ms
        /usr/bin/wmem 0x8002002 0x5A 2  # pull up LTE reset pin

        count=0
        while [ $count -lt 60 ]
        do                                  
            isPowerOn z
            retV=$?
            if [ $retV -eq 0 ]; then    
                count=`expr $count + 1`
                ECHO "wait $count seconds ..."
                sleep 1
            else     
                turnLEDOrange
                break
            fi                        
        done

        if [ $count -eq 60 ]; then
            ECHO "wait module down time out." # not turn led red
            EXIT 1 $*
        fi

        count=0
        while [ $count -lt 60 ]
        do                                  
            isPowerOn z
            retV=$?
            if [ $retV -eq 1 ]; then      
                count=`expr $count + 3`
                ECHO "wait $count seconds ..."
                sleep 3
            else     
                turnLEDOn
                EXIT 0 $*      
            fi                        
        done
        ECHO "reset wait module up time out."
        EXIT 2 $*
    else
        ECHO "module off, won't reset."
        EXIT 3 $*
    fi
}

function startPpp(){
    powerOn z
    echo 1 > /proc/sys/net/ipv4/ip_forward
    ECHO "start pppd, wait $PPP_IF up..."
    generatePppFiles
    pppd $PPP_DATA call $PPP_IF  2>&1  &
    #pppd call quectel-ppp >/dev/null 2>&1  &
    count=0
    bl=false
    while  [ $count -lt 30 ]; do
        if ifconfig | grep $PPP_IF; then
            ECHO "config NAT service"
            iptables -t nat -A POSTROUTING -o $PPP_IF -j MASQUERADE
            bl=true
            break
        else
            count=`expr $count + 2`
            ECHO "wait $count seconds ..."
            sleep 2
        fi
    done
    if [[ ${bl} == true ]]; then EXIT 0 ; fi
    ECHO "ppp dail time out."
    turnLEDOrange
    EXIT 1 $*
}

function stopPpp(){
    ECHO "start pppd, wait $PPP_IF up..."
    killPidFromPidFile $PID_FILE "pppd" 0
    retV=$?
    EXIT $retV
}

function killPidFromPidFile(){
    if [ ! -e $1 ];then
        ECHO "pid file:$1 not found!"
        return 1 
    fi
    pid=-1
    while read line; do 
        pid=$line;
        # ECHO "found pid:$pid"
        break
    done < $1
    if [ $pid -eq -1 ]; then
        ECHO "pid not found in pid file."
        return 1
    else
        ECHO "found pid:$pid for process:$2"
        killByPidAndWait $pid $3
        return $pid
    fi
}

function turnLEDOff(){
    # ECHO "turn off LED!"
    $W_MEM $LED_ADDR 0x00 2
    $W_MEM $LED_ADDR2 0x00 2
}

function turnLEDOn(){
    # ECHO "turn on LED!"
    $W_MEM $LED_ADDR 0x02 2
    $W_MEM $LED_ADDR2 0x02 2
}

function turnLEDred(){
    # ECHO "turn LED red!"
    $W_MEM $LED_ADDR 0x01 2
    $W_MEM $LED_ADDR2 0x01 2
}

function turnLEDOrange(){
    $W_MEM $LED_ADDR 0x03 2
    $W_MEM $LED_ADDR2 0x03 2
}

function ECHO(){ echo -e "\e[0;34;1m$key:\e[0m $1";}
function EXIT(){ exitV=$1; shift; if [ $# -eq 0 ]; then exit $exitV; else return $exitV; fi }

function usage(){
    echo "===$key management script==="
    echo "usage: $0 args"
    
    echo "------- Listing args ------"
    echo "   -c     : check $key is power on"
    echo "   -on    : power on $key"
    echo "   -off   : power off $key"
    echo "   -reset : reset module if power on"
    echo "   -sppp  : power on and start dial ppp(point-to-point protocol)"
    echo "   -eppp  : stop dial ppp(point-to-point protocol),not remove device"
    echo "   -gcnf  : generate configuration files"
}

function parse-opt(){
    while [ -n "$1" ];
    do
        case $1 in
           "-c")      isPowerOn;   break ;;
           "-on")     powerOn  ;   break ;;
           "-off")    powerOff ;   break ;;
           "-reset")  resetModule; break ;;
           "-sppp")   startPpp;    break ;;
           "-eppp")   stopPpp;     break ;;
           "-gcnf")   generatePppFiles; break;;
            *)        usage; exit 10;    ;;
        esac
    done
}

if [ $# -eq 0 ]; then usage; exit 10; fi
parse-opt $@



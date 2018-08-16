#!/bin/bash

#
# Copyright (c) 2018 Nokia
#
#  All rights reserved. 
#
#  Contributors:

#

# DO NOT modify exit code, script exit value used by other programe
# function "EXIT" must be at last line if call with "$*"

#TODO how to check power on off

key="6LOWPAN"
W_MEM="/usr/bin/wmem"
R_MEM="/usr/bin/rmem"
REL_FILE="/sys/bus/spi/drivers/adf7030"  #driver
# POWER_ADDR="0x8002000"
LED_ADDR="0x80000AA"

WPAN_IF="wpan0"
LOWPAN_IF="lowpan0"

PAN_ID="0xabcd"
SHORT_ADDR="0x0001"
LOWPAN_IP="2000::1/64"


function isPowerOn(){
    if [ ! -e $REL_FILE ]; then 
        ECHO "[status] off! Missing driver."
        turnLEDOff
        EXIT 1 $*
    fi
    if [ ! -e /sys/class/net/$WPAN_IF ]; then
        ECHO "[status] off! $WPAN_IF not exist."
        turnLEDOrange
        EXIT 1 $*
    elif [ ! -e /sys/class/net/$LOWPAN_IF ]; then
        ECHO "[status] off! $LOWPAN_IF not exist."
        turnLEDOrange
        EXIT 1 $*
    else
        ECHO "[status] on!"
        turnLEDOn
        EXIT 0 $*
    fi
}

function execCmd(){
    cmd=$1
    shift
    eval $cmd
    if [ ${?} -eq 1 ]; then
         ECHO "$cmd ...[FAIL]"
         turnLEDred
         EXIT 1 $*
    else
        ECHO "$cmd ...[PASS]"
    fi
}

function execCmdAnyWay(){
    cmd=$1
    shift
    eval $cmd
    if [ ${?} -eq 1 ]; then
         ECHO "$cmd ...[FAIL]"
         turnLEDOrange
    else
        ECHO "$cmd ...[PASS]"
    fi
}

function powerOn(){
    isPowerOn z
    retV=$?
    if [ $retV -ne 0 ]; then
        execCmd "ifconfig $WPAN_IF > /dev/null 2>&1"
        execCmd "iwpan dev $WPAN_IF info > /dev/null 2>&1"
        execCmd "iwpan dev $WPAN_IF set pan_id $PAN_ID > /dev/null 2>&1"
        execCmd "iwpan dev $WPAN_IF set short_addr $SHORT_ADDR > /dev/null 2>&1"
        execCmd "ip link add link $WPAN_IF name $LOWPAN_IF type lowpan > /dev/null 2>&1"
        execCmd "ifconfig $WPAN_IF up"
        execCmd "ifconfig $LOWPAN_IF up"
        execCmd "ip address add $LOWPAN_IP dev $LOWPAN_IF"
        ECHO "turn on successfully."
        turnLEDOn
        ECHO "====INFO===="
        ECHO "WPAN  :$WPAN_IF PAN_ID : $PAN_ID SHORT_ADDR: $SHORT_ADDR"
        ECHO "LOWPAN:$LOWPAN_IF IPV6_ADDR: $LOWPAN_IP"
        EXIT 0 $*
    else
        ECHO "already turned on."
        turnLEDOn
        EXIT 0 $*
    fi       
}

function powerOff(){
    isPowerOn z
    retV=$?
    if [ $retV -ne 1 ]; then
        execCmdAnyWay "ifconfig $WPAN_IF > /dev/null 2>&1"
        execCmdAnyWay "iwpan dev $WPAN_IF info > /dev/null 2>&1"
        execCmdAnyWay "ifconfig $LOWPAN_IF down > /dev/null 2>&1"
        execCmdAnyWay "ifconfig $WPAN_IF down > /dev/null 2>&1"
        execCmdAnyWay "ip link delete $LOWPAN_IF > /dev/null 2>&1"
        ECHO "turn off successfully."
        turnLEDOff
        EXIT 0 $*
    else
        ECHO "already turned off."
        EXIT 0 $*
    fi
}

function setPANID(){
    v=$1
    ECHO "set PAN_ID:$v"
    PAN_ID=$v
}

function setSHORTADDR(){
    v=$1
    if [ ${v:0:2} == "0x" ];then
        if [[ ${v#*x} =~ ^[a-fA-F0-9]+$ ]]; then
            ECHO "set SHORT_ADDR:$v"
            SHORT_ADDR=$v
        else
            ECHO "format error:$v"
        fi
    else
        ECHO "format error:$v"
    fi

}

function setLOWPANIP(){
    v=$1
    ECHO "set LOWPAN_IP:$v"
    LOWPAN_IP=$v
}

function turnLEDOff(){
    # ECHO "turn off LED!"
    $W_MEM $LED_ADDR 0x00 2
}

function turnLEDOn(){
    # ECHO "turn on LED!"
    $W_MEM $LED_ADDR 0x02 2
}

function turnLEDred(){
    # ECHO "turn LED red!"
    $W_MEM $LED_ADDR 0x01 2
}

function turnLEDOrange(){
    $W_MEM $LED_ADDR 0x03 2
}

function ECHO(){ echo -e "\e[0;34;1m$key:\e[0m $1";}
function EXIT(){ exitV=$1; shift; if [ $# -eq 0 ]; then exit $exitV; else return $exitV; fi }

function usage(){
    echo "===$key management script==="
    echo "usage: $0 args"
    
    echo "------- Listing args ------"
    echo "   -c     : check $key is power on"
    echo "   -on    : power on $key"
    echo "       -p : set PAN ID for power on, default:$PAN_ID"
    echo "       -s : set SHORT_ADDR for power on, default:$SHORT_ADDR"
    echo "       -i : set LOWPAN_IP for power on, default:$LOWPAN_IP"
    echo "   -off   : power off $key"
}

function parse-opt(){
    while [ -n "$1" ];
    do
        case $1 in
           "-c")      isPowerOn; break ;;
           "-on")  shift;
            while [ -n "$1" ]; do 
                case $1 in
                "-p") shift; setPANID $1;     shift; ;;   #PAN_ID
                "-s") shift; setSHORTADDR $1; shift; ;;   #SHORT_ADDR
                "-i") shift; setLOWPANIP $1;  shift; ;;   #LOWPAN_IP
                   *) shift;
                esac
            done
            powerOn; 
            break ;;
           "-off")    powerOff; break ;;
            *)  usage; exit 10; ;;
        esac
    done
}

if [ $# -eq 0 ]; then usage; exit 10; fi
parse-opt $@



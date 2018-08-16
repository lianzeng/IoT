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

key="GNSS"
W_MEM="/usr/bin/wmem"
R_MEM="/usr/bin/rmem"
REL_FILE="/dev/ttyUSB1"

RESET_ADDR="0x8002002"
POWER_ADDR="0x8002008"
LED_ADDR="0x80000A6"
LED_ADDR2="0x80000A2"

 # |--microcom /dev/ttyUSB2   >> AT+QGPS=1    //start  GPS/LTE
 # |--microcom /dev/ttyUSB2   >> AT+QGPSLOC?  //check position
 # |--microcom /dev/ttyUSB2   >> AT+QGPSEND   //stop GPS


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
    if [ $retV -ne 0 ]; then
        $W_MEM $RESET_ADDR 0x5A 2  # pull up LTE reset pin
        $W_MEM $POWER_ADDR 0x01 2  # pull down LTE power pin
        usleep 200000              # Wait for about 200ms
        $W_MEM $POWER_ADDR 0x00 2  # pull up LTE power pin
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
                ECHO "turn on successfully.This also turn on MOBILE module."
                turnLEDOn
                EXIT 0 $*
            fi
        done
        ECHO "turn on time out."
        turnLEDred
        EXIT 1 $*
    else
        ECHO "already turned on."
        EXIT 0 $*
    fi	
}

function powerOff(){
    isPowerOn z
    retV=$?
    if [ $retV -ne 1 ]; then
        # $W_MEM $POWER_ADDR 0x01 2  # pull down LTE power pin
        # usleep 800000              # Wait for about 800ms
        # $W_MEM $POWER_ADDR 0x00 2  # pull up LTE power pin

        # count=0
        # while [ $count -lt 60 ]
        # do
        #     isPowerOn z
        #     retV=$?
        #     if [ $retV -ne 1 ]; then
        #         count=`expr $count + 7`
        #         ECHO "wait $count seconds ..."
        #         sleep 7
        #     else
        #         ECHO "turn off successfully.This also turn off MOBILE module."
        #         turnLEDOff
        #         EXIT 0 $*
        #     fi
        # done
        # ECHO "turn off time out."
        # turnLEDred
        # EXIT 1 $* 
        #TODO ....
        ECHO "turn off successfully.This only turn off LED."
        turnLEDOff
        EXIT 0 $*
    else
        ECHO "already turned off.(LED only)"
        EXIT 0 $*
    fi
}



function turnLEDOff(){
    # ECHO "turn off LED!"
    $W_MEM $LED_ADDR 0x00 2
    # $W_MEM $LED_ADDR2 0x00 2
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
}

function parse-opt(){
    while [ -n "$1" ];
    do
        case $1 in
           "-c")      isPowerOn; break ;;
           "-on")     powerOn  ; break ;;
           "-off")    powerOff ; break ;;
            *)        usage;  exit 10; ;;
        esac
    done
}

if [ $# -eq 0 ]; then usage; exit 10; fi
parse-opt $@


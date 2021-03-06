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

key="ZIGBEE"
W_MEM="/usr/bin/wmem"
R_MEM="/usr/bin/rmem"
# REL_FILE="/sys/bus/spi/drivers/adf7030"  #driver
# POWER_ADDR="0x8002000"
LED_ADDR="0x80000A8"


function isPowerOn(){
	# TODO add real implmentation
	P_VAL=`$R_MEM $LED_ADDR 2 | grep "r" | awk 'BEGIN { FS=": "} END {print $3}'`
	if [ ${P_VAL: -2} == 02 ]; then
        ECHO "[status] on!"
        EXIT 0 $*
    elif [ ${P_VAL: -2} == 00 ]; then
        ECHO "[status] off!"
        EXIT 1 $*
    else 
        ECHO "[status] EXCEPTION while query status!($P_VAL)"
        turnLEDred
        EXIT 2 $*
    fi
}


function powerOn(){
    isPowerOn z
    retV=$?
    if [ $retV -ne 0 ]; then
    	ECHO "turn on successfully. Turn on LED only,(TODO) add reset via UART..."
        turnLEDOn
        EXIT 0 $*
    else
        ECHO "already turned on."
        EXIT 0 $*
    fi
}



function powerOff(){
	isPowerOn z
    retV=$?
    if [ $retV -ne 1 ]; then
		ECHO "turn off successfully. Turn off LED only,(TODO) add reset via UART..."
		turnLEDOff
		EXIT 0 $*
	else
		ECHO "already turn off."
        turnLEDOff
        EXIT 0 $*
    fi
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
    echo "   -off   : power off $key"
}

function parse-opt(){
    while [ -n "$1" ];
    do
        case $1 in
           "-c")    isPowerOn; break ;;
           "-on")   powerOn  ; break ;;
           "-off")  powerOff ; break ;;
            *)  usage; exit 10; ;;
        esac
    done
}

if [ $# -eq 0 ]; then usage; exit 10; fi
parse-opt $@







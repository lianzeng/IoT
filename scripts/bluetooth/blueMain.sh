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

key="BLUETOOTH"
W_MEM="/usr/bin/wmem"
R_MEM="/usr/bin/rmem"
FIRMWARE_FILE="TIInit_6.12.26.bts"
FIRMWARE_PATH="/lib/firmware/ti-connectivity"
REL_FILE="/dev/ttyS4"
NSHUTD_ADDR="0x8000032"
LED_ADDR="0x80000AC"
HCIX="hci0"
DEFAULT_PAIR_KEY="0000"
BRIDGE_NAME="br0"
BRIDGE_IP="192.168.3.131"

GLOBAL_CONF="/etc/global.conf"
G_DEV_ID_K="G_DEV_ID"
G_DEV_ID_V="default"



function isPowerOn(){
    P_VAL=`$R_MEM $NSHUTD_ADDR 2 | grep "r" | awk 'BEGIN { FS=": "} END {print $3}'`
    if [ ${P_VAL: -2} == a5 ]; then
        hciconfig $HCIX >/dev/null 2>&1
        if [ $? -eq 0 ]; then
            ECHO "[status] on!"
            turnLEDOn
            EXIT 0 $*
        else
            ECHO "[status] EXCEPTION initialized may failed!"
            turnLEDred
            EXIT 1 $*
        fi
    elif [ ${P_VAL: -2} == 5a ]; then
        ECHO "[status] off!"
        turnLEDOff
        EXIT 1 $*
    elif [ ${P_VAL: -2} == 00 ]; then
        ECHO "[status] off!(initial)"
        turnLEDOff
        EXIT 1 $*
    else
        ECHO "[status] EXCEPTION while query status!($P_VAL)"
        turnLEDred
        EXIT 2 $*
    fi
}

function isProcessExist(){
    EX=`ps | grep $1 | grep -v grep`
    if [ -z "$EX" ]; then 
        ECHO "process $1 not exist."
        return 1
    else
        ECHO "process $1 exist."
        return 0
    fi
}

function killProcess(){
    isProcessExist $1
    retV=$?
    if [ $retV -eq 0 ]; then
        killall -9 $1
        ECHO "killProcess $1"
    fi
}

function powerOn(){
    isPowerOn z
    retV=$?
    if [ $retV -ne 0 ]; then
        if [ ! -d $FIRMWARE_PATH ]; then 
           mkdir -p $FIRMWARE_PATH
        fi

        if [ ! -e $FIRMWARE_PATH/$FIRMWARE_FILE ]; then
            if [ ! -e /usr/firmwares/$FIRMWARE_FILE ]; then
                ECHO "(ERR) no firmware file found."
                turnLEDred
                EXIT 1 $*
            fi
            ln -s /usr/firmwares/$FIRMWARE_FILE $FIRMWARE_PATH/$FIRMWARE_FILE
        fi

        diff /usr/bluez/bluetooth.conf /etc/dbus-1/system.d/bluetooth.conf >/dev/null
        if [ $? -eq 0 ]; then
            ECHO "no configuration file need update."
        else 
            cp /usr/bluez/bluetooth.conf /etc/dbus-1/system.d/bluetooth.conf
            killall -1 dbus-daemon
        fi

        killProcess hciattach

        $W_MEM $NSHUTD_ADDR 5a 2 
        # sleep 2
        $W_MEM $NSHUTD_ADDR a5 2 
        sleep 1

        ECHO "initializing module ..."
        hciattach $REL_FILE texas 115200 noflow
        ECHO "initializd done."

        #check if start successful
        hciconfig $HCIX >/dev/null 2>&1
        if [ $? -eq 0 ]; then
            ECHO "turn on successfully."
            turnLEDOn
            EXIT 0 $*
        else
            ECHO "turn on failed."
            turnLEDred
            EXIT 1 $*
        fi
    else
        # return value must be 0
        ECHO "already turned on."
        EXIT 0 $*
    fi
}

function powerOff(){
    isPowerOn z
    retV=$?
    if [ $retV -ne 1 ]; then
        $W_MEM $NSHUTD_ADDR 5a 2
        killProcess ble_agent
        killProcess bluetoothd
        killProcess hciattach
        if [ -d /sys/class/net/$BRIDGE_NAME ]; then
            ifconfig $BRIDGE_NAME down
            brctl delbr $BRIDGE_NAME
            ECHO "force delete $BRIDGE_NAME"
        fi
        ECHO "turn off successfully."
        EXIT 0 $*
    else
        ECHO "already turned off."
        EXIT 0 $*
    fi
}


function startBLUBLE(){
    powerOn z
    killProcess ble_agent
    killProcess bluetoothd
    sleep 2
    ECHO "start bluetoothd..."
    /usr/libexec/bluetooth/bluetoothd -C -E &

    sleep 1
    ECHO "add SP & NAP"
    sdptool add SP >/dev/null
    sdptool add NAP >/dev/null

    if [ -d /sys/class/net/$BRIDGE_NAME ]; then
        ifconfig $BRIDGE_NAME down
        brctl delbr $BRIDGE_NAME
        ECHO "force delete $BRIDGE_NAME"
    fi
    ECHO "add bridge $BRIDGE_NAME"
    brctl addbr $BRIDGE_NAME 
    ifconfig $BRIDGE_NAME $BRIDGE_IP
    bridge_mac=`cat /sys/class/net/br0/address`
    ECHO "bridge [MAC]$bridge_mac [IP]$BRIDGE_IP "
    
    # hci_addr=`hciconfig  hci0 | grep Address | sed "s/.*Address:[[:space:]]\+\([a-z0-9A-F\:]*\).*/\1/" | tr '[:lower:]' '[:upper:]'`

    G_DEV_ID_V=`grep $G_DEV_ID_K $GLOBAL_CONF | awk 'BEGIN {FS="="} END {print $2}'`
    ECHO "read global dev id($G_DEV_ID_V) from file."
   
    HUM_NAME=`cat /etc/hostname`_$G_DEV_ID_V

    ECHO "set $HCIX name:$HUM_NAME"
    hciconfig $HCIX name $HUM_NAME

    ECHO "set $HCIX piscan"
    hciconfig $HCIX piscan      #Enable Page and Inquiry scan

    ECHO "set $HCIX sspmode 0"
    hciconfig $HCIX sspmode 0   #Get/Set Simple Pairing Mode

    ECHO "start ble_agent and set passkey..."
    ble_agent $DEFAULT_PAIR_KEY &           #default passkey is 0000
    EXIT 0 $*
}

function tryFix(){
    isPowerOn z
    retV=$?
    if [ $retV -ne 0 ]; then
        ECHO "please power on first."
    fi
    hciconfig $HCIX reset
    ECHO "reset adapter, you can continue."
    sleep 2
    EXIT 0 $*
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
    echo "   -bluble: power on $key and force restart blu & ble"
    echo "   -fix   : check if adapter is working, and try to fix it.(bluble)"
}

function parse-opt(){
    while [ -n "$1" ];
    do
        case $1 in
           "-c")      isPowerOn;   break ;;
           "-on")     powerOn  ;   break ;;
           "-off")    powerOff ;   break ;;
           "-bluble") startBLUBLE; break ;;
           "-fix")    tryFix;      break ;;
            *)        usage; exit 10;    ;;
        esac
    done
}

if [ $# -eq 0 ]; then usage; exit 10; fi
parse-opt $@

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

key="WIFI"
W_MEM="/usr/bin/wmem"
LED_ADDR="0x80000A0"
MODULE_ADDR="0x8002010"
WIFI_IF="wlan0"

ROOT=$(dirname $(readlink -f "$0"))
REL_FILE="/sys/bus/mmc/devices/mmc2:0001"
REL_FILE2="/sys/class/net/$WIFI_IF"

IP_ADDR="192.168.20.1"          #default value
NETMASK="255.255.255.0"         #default value, TODO need update via conf
UDHCPD_IP_START="192.168.20.10" #default value
UDHCPD_IP_END="192.168.20.50"   #default value

CONFIG_DIR="/etc/sysconfig/network-scripts"
CONFIG_FILE="ifcfg-$WIFI_IF"

HOSTAPD_PROC="hostapd"
HOSTAPD_CONF="/etc/hostapd-$WIFI_IF.conf"
HOSTAPD_PID_FILE="/var/run/hostapd-$WIFI_IF.pid"
AP_SSID_PREFIX=`cat /etc/hostname`_
AP_PWD="testKEYS"

UDHCPD_PROC="udhcpd"
UDHCPD_CONF="/etc/udhcpd-$WIFI_IF.conf"
UDHCPD_PID_FILE="/var/run/udhcpd-$WIFI_IF.pid"

UDHCPC_PROC="udhcpc"
UDHCPC_PID_FILE="/var/run/udhcpc-$WIFI_IF.pid"

WPASUP_PROC="wpa_supplicant"
WPASUP_CONF="/etc/wpa_supplicant-$WIFI_IF.conf"
WPASUP_PID_FILE="/var/run/wpa_supplicant-$WIFI_IF.pid"

STA_SSID="testssid"
STA_PSK="testpwd123"   #must be at lest 8 char

GLOBAL_CONF="/etc/global.conf"
G_DEV_ID_K="G_DEV_ID"
G_DEV_ID_V="default"

OPTS=""


function isPowerOn(){    
    if [[ -h $REL_FILE && -d $REL_FILE2 ]]; then
        ECHO "[status] on!"
        turnLEDOn
        EXIT 0 $*
    else
        ECHO "[status] off!"
        turnLEDOff
        EXIT 1 $*
    fi 
}


function generateIPConf(){
    if [  -e $CONFIG_DIR/$CONFIG_FILE ]; then
        ECHO "try get ip from $CONFIG_DIR/$CONFIG_FILE"
        IP_ADDR=`cat $CONFIG_DIR/$CONFIG_FILE | grep IPADDR | awk 'BEGIN { FS="="} END {print $2}'`
        ECHO "IP in configuration file:$IP_ADDR"
    else
        mkdir -p $CONFIG_DIR
cat > $CONFIG_DIR/$CONFIG_FILE <<EOF
# Networking Interface
# Generate by wifiMain.sh
DEVICE=$WIFI_IF
NAME=$WIFI_IF
TYPE=WIFI
ONBOOT=yes
BOOTPROTO=static
IPADDR=$IP_ADDR
PREFIX=24
DEFROUTE=no

#Wireless configuration
MODE=Master
EOF
     ECHO "generate $CONFIG_FILE with default IP:$IP_ADDR"
    fi
}


function ensureHostapdConf(){
    if [ ! -e $HOSTAPD_CONF ]; then
        ECHO "file not found,generate it.$HOSTAPD_CONF"
        G_DEV_ID_V=`grep $G_DEV_ID_K $GLOBAL_CONF | awk 'BEGIN {FS="="} END {print $2}'`
        echo "read global dev id($G_DEV_ID_V) from file."
        # AP_SSID_SUFFIX=`ip -o link show $WIFI_IF | awk 'BEGIN { FS=" " } END { print $11}'`
        AP_SSID_SUFFIX=$G_DEV_ID_V
        ECHO "will set AP name:$AP_SSID_PREFIX$AP_SSID_SUFFIX with pwd $AP_PWD"
cat > $HOSTAPD_CONF <<EOF
# $HOSTAPD_CONF
# Generate by wifiMain.sh

interface=wlan0
driver=nl80211

# SSID to use. This will be the "name" of the accesspoint
ssid=$AP_SSID_PREFIX$AP_SSID_SUFFIX

# basic operational settings
hw_mode=g

wme_enabled=1
ieee80211n=1
ht_capab=[SHORT-GI-20]
channel=6

# Logging and debugging settings: more of this in original config file
logger_syslog=-1
logger_syslog_level=2
logger_stdout=-1
logger_stdout_level=2
dump_file=/tmp/hostapd.dump

# WPA settings. We'll use stronger WPA2
# bit0 = WPA,  bit1 = IEEE 802.11i/RSN (WPA2) (dot11RSNAEnabled)
wpa=2

# Preshared key of between 8-63 ASCII characters.
# If you define the key in here, make sure that the file is not readable
# by anyone but root. Alternatively you can use a separate file for the
# key; see original hostapd.conf for more information.
wpa_passphrase=$AP_PWD

# Key management algorithm. In this case, a simple pre-shared key (PSK)
wpa_key_mgmt=WPA-PSK

# The cipher suite to use. We want to use stronger CCMP cipher.
wpa_pairwise=CCMP

# Change the broadcasted/multicasted keys after this many seconds.
wpa_group_rekey=600

# Change the master key after this many seconds. Master key is used as a basis
# (source) for the encryption keys.
wpa_gmk_rekey=86400

# Send empty SSID in beacons and ignore probe request frames that do not
# specify full SSID, i.e., require stations to know SSID.
# default: disabled (0)
# 1 = send empty (length=0) SSID in beacon and ignore probe request for
#     broadcast SSID
# 2 = clear SSID (ASCII 0), but keep the original length (this may be required
#     with some clients that do not support empty SSID) and ignore probe
#     requests for broadcast SSID
ignore_broadcast_ssid=0
EOF
    else
        ECHO "file exist.$HOSTAPD_CONF"
    fi
}

function ensureUdhcpdConf(){
    if [ ! -e $UDHCPD_CONF ]; then
        ECHO "file not found,generate it.$UDHCPD_CONF"
cat > $UDHCPD_CONF <<EOF
# Generate by wifiMain.sh
start $UDHCPD_IP_START
end $UDHCPD_IP_END
interface $WIFI_IF
pidfile /var/run/udhcpd-$WIFI_IF.pid
max_leases 41
auto_time 0
decline_time 864000
conflict_time 864000
offer_time 864000
min_lease 864000
opt subnet $NETMASK
opt router $IP_ADDR
opt lease 864000
opt dns $IP_ADDR
EOF
    else
        ECHO "file exist.$UDHCPD_CONF"
    fi
}


function ensureWPASUPConf(){
    if [ ! -e $WPASUP_CONF ]; then
        ECHO "file not found,generate it.$WPASUP_CONF"
cat > $WPASUP_CONF <<EOF
# Generate by wifiMain.sh
# allow frontend (e.g., wpa_cli) to be used by all users in 'wheel' group
ctrl_interface=/var/run/wpa_supplicant
ctrl_interface_group=wheel

# home network; allow all valid ciphers
network={
    mode=0
    ssid="$STA_SSID"
    scan_ssid=1
    key_mgmt=WPA-PSK
    scan_freq=2412 2417 2422 2427 2432 2437 2442 2447 2452 2457 2462 2467 2472
    bgscan=""
    psk="$STA_PSK"
}
EOF
    else
        ECHO "file exist.$WPASUP_CONF"
    fi
}

function powerOn(){
    bl=false
    isPowerOn z
    retV=$?
    if [ $retV -ne 0 ]; then
        $W_MEM $MODULE_ADDR 0x1 2
        count=0
        while [ $count -lt 30 ]
        do
            isPowerOn z
            retV=$?
            if [ $retV -gt 0 ]; then
                count=`expr $count + 1`
                ECHO "wait $count seconds ..."
                sleep 1
            else
                sleep 2
                ifconfig $WIFI_IF up 
                # generateIPConf
                #ifconfig $WIFI_IF $IP_ADDR netmask $NETMASK up
                ECHO "turn on successfully."
                turnLEDOn
                bl=true
                break
            fi
        done
        if [[ ${bl} == true ]]; then 
            EXIT 0 $*;
        else
            ECHO "turn on time out."
            turnLEDred
            $W_MEM $MODULE_ADDR 0x0 2
            EXIT 1 $*
        fi
    else
        ECHO "already turned on."
        EXIT 0 $*
    fi   
}

function powerOff(){
    bl=false
    isPowerOn z
    retV=$?
    if [ $retV -ne 1 ]; then
        killPidFromPidFile $UDHCPD_PID_FILE   $UDHCPD_PROC  1
        killPidFromPidFile $HOSTAPD_PID_FILE  $HOSTAPD_PROC 3
        killPidFromPidFile $WPASUP_PID_FILE   $WPASUP_PROC  2
        killPidFromPidFile $UDHCPC_PID_FILE   $UDHCPC_PROC  0

        $W_MEM $MODULE_ADDR 0x0 2
        count=0
        while [ $count -lt 30 ]
        do
            isPowerOn z
            retV=$?
            if [ $retV -ne 1 ]; then
                count=`expr $count + 1`
                ECHO "wait $count seconds ..."
                sleep 1
            else
                ECHO "turn off successfully."
                turnLEDOff
                bl=true
                break
            fi
        done
        if [[ ${bl} == true ]]; then EXIT 0 $*; fi
        ECHO "turn off time out."
        turnLEDred
        EXIT 1 $*
    else
        ECHO "already turned off."
        EXIT 0 $*
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

# function isProcessExist(){
#     EX=`ps | grep $1 | grep -v grep`
#     if [ -z "$EX" ]; then 
#         ECHO "process $1 not exist."
#         return 1
#     else
#         ECHO "process $1 exist."
#         return 0
#     fi
# }

function showProcessLine(){
    EX=`ps | grep $1 | grep -v grep`
    if [ -z "$EX" ]; then 
        turnLEDred
        ECHO "process $1 not exist."
    else
        ECHO "process $EX"
    fi
}

# function killProcessAndWait(){
#     isProcessExist $1
#     retV=$?
#     if [ $retV -eq 0 ]; then
#         killall -9 $1
#         ECHO "killProcess $1 wait $2 second(s)"
#         sleep $2
#     fi
# }

function startAP(){
    powerOff z
    powerOn z
    #check again
    isPowerOn z
    retV=$?
    if [ $retV -ne 0 ]; then
        ECHO "turn on failed"
        turnLEDred
        EXIT 1 $*
    fi
    ensureHostapdConf
    ensureUdhcpdConf

    #-------open forward
    sleep 3 
    ECHO "open ipv4 forward"
    echo 1 > /proc/sys/net/ipv4/ip_forward
    if [ ! -d /sys/class/net/$WIFI_IF ]; then
        EXIT 1 $*
    fi
    #------hostapd
    killPidFromPidFile $HOSTAPD_PID_FILE $HOSTAPD_PROC 3 
    ECHO "start hostapd  $HOSTAPD_CONF"
    $HOSTAPD_PROC  -B  -P $HOSTAPD_PID_FILE $HOSTAPD_CONF 
    sleep 3
    if [[ $OPTS == "-ds" ]]; then    
        #-----udhcpd
        generateIPConf
        ifconfig $WIFI_IF $IP_ADDR netmask $NETMASK up

        killPidFromPidFile $UDHCPD_PID_FILE $UDHCPD_PROC 3
        ECHO "start udpcpd $UDHCPD_CONF"
        $UDHCPD_PROC -S -I $IP_ADDR $UDHCPD_CONF
    else
        ECHO "skip start udhcpd."
    fi

    ECHO "check application status...."
    showProcessLine $HOSTAPD_PROC
    showProcessLine $UDHCPD_PROC
    
    ifconfig $WIFI_IF $IP_ADDR netmask $NETMASK up
    ECHO "open NAT"
    iptables -t nat -A POSTROUTING -o ppp0 -j MASQUERADE
}

function startSTA(){
    powerOff z
    powerOn z

    #check again
    isPowerOn z
    retV=$?
    if [ $retV -ne 0 ]; then
        ECHO "turn on failed"
        turnLEDred
        EXIT 1 $*
    fi

    ensureWPASUPConf
    killPidFromPidFile $WPASUP_PID_FILE $WPASUP_PROC 1
    #nl80211  iw dev info
    ECHO "run $WPASUP_PROC with $WPASUP_CONF"
    $WPASUP_PROC -u -B -D  nl80211 -i wlan0 -e $ROOT/entropy.bin  -c $WPASUP_CONF -P $WPASUP_PID_FILE
    showProcessLine $WPASUP_PROC
    if [[ $OPTS == "-dc" ]]; then 
        $UDHCPC_PROC -i $WIFI_IF -p $UDHCPC_PID_FILE
        sta_ip=`ip -o -4 addr show $WIFI_IF | awk 'BEGIN { FS=" "} END {print $4}'`
        ECHO "ipaddr of $WIFI_IF: $sta_ip"
    else
        ECHO "skip start dhcp client."
        ECHO "You can configurate IPAddr by manual."
    fi
}

function generateConf(){
    ECHO "generate configuration files."
    generateIPConf
    ensureHostapdConf
    ensureUdhcpdConf
    ensureWPASUPConf
}

function removeMod(){
    ECHO "remove module:$1"
    rmmod $1
}

function loadMod(){
    ECHO "load module:$1"
    modprobe $1
}

function loadCore(){
    if ! mount | grep debugfs > /dev/null; then
        ECHO "Mount debugfs"
        mount -t debugfs none /sys/kernel/debug
    fi
    loadMod wl18xx
    loadMod wlcore_sdio
}

function unloadCore(){
    powerOff z
    removeMod wlcore_sdio
    removeMod wl18xx
    removeMod wl12xx
    removeMod wlcore
    removeMod mac80211
    removeMod cfg80211
    removeMod compat
}

function turnLEDOff(){
    # echo "$key stopped. turn LED off!"
    $W_MEM $LED_ADDR 0x00 2
}

function turnLEDOn(){
    # echo "$key works. turn LED on!"
    $W_MEM $LED_ADDR 0x02 2
}

function turnLEDred(){
    # echo "$key exception. turn LED red!"
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
    echo "   -c         : check $key is power on"
    echo "   -on        : power on $key"
    echo "   -off       : power off $key, stop AP and STA"
    echo "   -ap  [-ds]  : configurate as AP(WPA-PSK), -ds: means with dhcpserver, or without it"
    echo "   -sta [-dc]  : configurate as STA mode, -dc: means with dhcpclient, or without it"
    echo "   -gcnf      : generate configuration files"
    echo "   -loadcore  : this is only for debug"
    echo "   -unloadcore: this is only for debug"
}

function parse-opt(){
    while [ -n "$1" ];
    do
        case $1 in
           "-c")     isPowerOn  ; break ;;
           "-on")    powerOn    ; break ;;
           "-off")   powerOff   ; break ;;
           "-ap")    shift; OPTS=$*; startAP; break ;;
           "-sta")   shift; OPTS=$*; startSTA; break ;;
           "-gcnf")  generateConf; break ;;
           "-loadcore")   loadCore ; break ;;
           "-unloadcore") unloadCore ; break ;;

            *)  usage; exit 10; ;;
        esac
    done
}

if [ $# -eq 0 ]; then usage; exit 10; fi
parse-opt $@


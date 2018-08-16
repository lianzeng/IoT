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

key="ETHERNET"

MAC_PREFIX="F8:5C:4D"
ETH_IF="eth0"

IP_ADDR="192.168.1.100"

NETMASK="255.255.255.0"        #default value, TODO need update via conf
UDHCPD_IP_START="192.168.1.10" #default value
UDHCPD_IP_END="192.168.1.50"   #default value

GLOBAL_CONF="/etc/global.conf"
G_DEV_ID_K="G_DEV_ID"
G_DEV_ID_V=""

CONFIG_DIR="/etc/sysconfig/network-scripts"
CONFIG_FILE="ifcfg-$ETH_IF"

UDHCPD_PROC="udhcpd"
UDHCPD_CONF="/etc/udhcpd-$ETH_IF.conf"
UDHCPD_PID_FILE="/var/run/udhcpd-$ETH_IF.pid"

UDHCPC_PROC="udhcpc"
UDHCPC_PID_FILE="/var/run/udhcpc-$ETH_IF.pid"


OPTS=""


function ensureIPConf(){
    if [ -e $CONFIG_DIR/$CONFIG_FILE ]; then
        ECHO "try get ip from $CONFIG_DIR/$CONFIG_FILE"
        IP_ADDR=`cat $CONFIG_DIR/$CONFIG_FILE | grep IPADDR | awk 'BEGIN { FS="="} END {print $2}'`
        ECHO "IP in configuration file:$IP_ADDR"
    else
        mkdir -p $CONFIG_DIR
cat > $CONFIG_DIR/$CONFIG_FILE <<EOF
# Networking Interface
DEVICE=$ETH_IF
NAME=$ETH_IF
TYPE=ETHERNET
ONBOOT=yes
BOOTPROTO=static
DEFROUTE=yes
IPADDR=$IP_ADDR
PREFIX=24
EOF
     echo "generate $CONFIG_FILE with default IP:$IP_ADDR"
    fi
}

function ensureUdhcpdConf(){
    if [ ! -e $UDHCPD_CONF ]; then
        ECHO "file not found,generate it.$UDHCPD_CONF"
cat > $UDHCPD_CONF <<EOF
# Generate by ethernetMain.sh
start $UDHCPD_IP_START
end $UDHCPD_IP_END
interface $ETH_IF
pidfile /var/run/udhcpd-$ETH_IF.pid
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


function generateConf(){
    ensureIPConf
    ensureUdhcpdConf
}

function configIP(){
	ensureIPConf  
    killPidFromPidFile $UDHCPD_PID_FILE $UDHCPD_PROC 0
    killPidFromPidFile $UDHCPC_PID_FILE $UDHCPC_PROC 0
    ifconfig $ETH_IF down

	G_DEV_ID_V=`grep $G_DEV_ID_K $GLOBAL_CONF | awk 'BEGIN {FS="="} END {print $2}'`
    ECHO "read global dev id($G_DEV_ID_V) from file."

	#make mac
    mac_suffix=${G_DEV_ID_V:0:2}:${G_DEV_ID_V:2:2}:${G_DEV_ID_V:4:2}
    eth_mac=$MAC_PREFIX:$mac_suffix
    ECHO "init for:$ETH_IF IP:$IP_ADDR MAC:$eth_mac"
    ifconfig $ETH_IF hw ether $eth_mac
    ifconfig $ETH_IF up
    #ifconfig $ETH_IF $IP_ADDR netmask $NETMASK up
    ethtool -A $ETH_IF rx on tx on  2>/dev/null
    
    if [[ $OPTS == "-dc" ]]; then 
        $UDHCPC_PROC -i $ETH_IF -p $UDHCPC_PID_FILE
        #add timeout

    elif [[ $OPTS == "-ds" ]]; then
        ifconfig $ETH_IF $IP_ADDR netmask $NETMASK up  #TODO read PREFIX from file
        killPidFromPidFile $UDHCPD_PID_FILE $UDHCPD_PROC 0
        ECHO "start udpcpd $UDHCPD_CONF"
        $UDHCPD_PROC -S -I $IP_ADDR $UDHCPD_CONF
    else
        ifconfig $ETH_IF $IP_ADDR netmask $NETMASK up
        ECHO "configure static IP only."
    fi
    ip=`ip -o -4 addr show $ETH_IF | awk 'BEGIN { FS=" "} END {print $4}'`
    ECHO "ipaddr of $ETH_IF: $ip"
    EXIT 0
}

function interfaceDown(){
    ECHO "interface down"
    killPidFromPidFile $UDHCPD_PID_FILE $UDHCPD_PROC 0
    killPidFromPidFile $UDHCPC_PID_FILE $UDHCPC_PROC 0
    ifconfig $ETH_IF 0
    ifconfig $ETH_IF down
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

# function killProcessAndWait(){
#     isProcessExist $1
#     retV=$?
#     if [ $retV -eq 0 ]; then
#         killall -9 $1
#         ECHO "killProcess $1 wait $2 second(s)"
#         sleep $2
#     fi
# }

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

function ECHO(){ echo -e "\e[0;34;1m$key:\e[0m $1";}
function EXIT(){ exitV=$1; shift; if [ $# -eq 0 ]; then exit $exitV; else return $exitV; fi }

function usage(){
    echo "===$key management script==="
    echo "usage: $0 args"
    
    echo "------- Listing args ------"
    echo "   -gcnf         : generate configuration files"
    echo "   -on [-dc|-ds] : configure IP for $ETH_IF, -dc: means with dhcpclient, -ds: means with dhcpserver"
    echo "   -off          : stop dhcpclient,dhcpserver,and interface down"
}

function parse-opt(){
    while [ -n "$1" ];
    do
        case $1 in
           "-gcnf") generateConf; break ;;
           "-on")   shift; OPTS=$*; configIP; break ;;
           "-off")  interfaceDown; break;;
            *)      usage; exit 10;    ;;
        esac
    done
}

if [ $# -eq 0 ]; then usage; exit 10; fi
parse-opt $@
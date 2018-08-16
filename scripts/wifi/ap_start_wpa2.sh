#!/bin/sh

########## variables ##########

WLAN=wlan0
HOSTAPD_PROC=/usb/sbin/hostapd
HOSTAPD_CONF=/etc/hostapd.conf
HOSTAPD_BIN_DIR=/usr/sbin
IP_ADDR=192.168.43.1
DHCP_CONF=/usr/share/wl18xx/udhcpd.conf
DHCP_CONF_PROC=udhcpd

########## body ##########



### start a hostapd if not running 
if [ ! -r $HOSTAPD_PROC ]
then
 $HOSTAPD_BIN_DIR/hostapd $HOSTAPD_CONF &
 sleep 3 
fi

### configure ip forewarding
echo 1 > /proc/sys/net/ipv4/ip_forward

### add WLAN interface, if not present
if [ ! -d /sys/class/net/$WLAN ]
then
  exit 1
fi



### configure ip
ifconfig $WLAN $IP_ADDR netmask 255.255.255.0 up

### start udhcpd server, if not started
if [ ! -r udhcpd ]; then
  udhcpd $DHCP_CONF
fi


### configure nat
iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE


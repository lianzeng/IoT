#!/bin/sh

# Check WLAN status
file="/sys/bus/mmc/devices/mmc2:0001"

# ifconfig wlan0 > /dev/null
# if [ ${?} -ne 0 ]; then
if [ ! -h $file ]; then
    # echo "WIFI already stopped. exit"
    exit 0
fi

# Power off wifi module

wmem 0x8002010 0x0 2

# Wait for wifi ready or 30 seconds passed.
count=0

while [ $count -lt 30 ]
do
    # ifconfig wlan0 > /dev/null
    # if [ ${?} -eq 0 ]; then
    if [ -h $file ]; then
        count=`expr $count + 1`
        # echo "wait $count seconds ..."
        sleep 1
    else
        # echo "WIFI stopped."
        # LED control
        /usr/bin/wmem 0x80000A0 0x00 2
        exit 0
    fi
done
# echo "Time out for wait wifi power off. exit"
exit 1


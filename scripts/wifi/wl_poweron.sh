#!/bin/sh

# Check WLAN status
file="/sys/bus/mmc/devices/mmc2:0001"
# ifconfig wlan0 > /dev/null
# if [ ${?} -eq 0 ]; then
if [ -h $file ]; then
    # echo "WIFI already running. exit"
    exit 0
fi

# Power on wifi module

wmem 0x8002010 0x1 2

# Wait for wifi ready or 30 seconds passed.
count=0

while [ $count -lt 30 ]
do
    # ifconfig wlan0 > /dev/null
    # if [ ${?} -ne 0 ]; then
    if [ ! -h $file ]; then
        count=`expr $count + 1`
        # echo "wait $count seconds ..."
        sleep 1
    else
        # echo "WIFI works."
        # LED control
        /usr/bin/wmem 0x80000A0 0x02 2
        exit 0
    fi
done
# echo "Time out for wait wifi ready. exit"
exit 1

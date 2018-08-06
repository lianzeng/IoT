#!/bin/sh




echo " $0 $1 "

#absolute path
dstfile="/opt/lwm2m/edgeTest.txt"
logfile="/opt/lwm2m/wget.log"

currTime=$(date +"%Y%m%d%H%M%S")
echo $currTime >> $logfile



#note: unset $http_proxy in host first

#wget http://52.80.95.56:8038/edgeTest.txt


#backup rather then rm ?
if [ -f $dstfile ];then
rm  $dstfile
fi

#echo "wget start"

#try 2 times
/usr/bin/wget -t 2 -O $dstfile  $1 &>> $logfile


exit $?

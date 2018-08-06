#source this .sh 
source  /opt/fsl-imx-x11/4.9.11-1.0.0/environment-setup-cortexa9hf-neon-poky-linux-gnueabi 

#echo $CC , then output :
#arm-poky-linux-gnueabi-gcc -march=armv7-a -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 --sysroot=/opt/fsl-imx-x11/4.9.11-1.0.0/sysroots/cortexa9hf-neon-poky-linux-gnueabi

echo $CXX
# then output:
#arm-poky-linux-gnueabi-g++ -march=armv7-a -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 --sysroot=/opt/fsl-imx-x11/4.9.11-1.0.0/sysroots/cortexa9hf-neon-poky-linux-gnueabi

#compile&link two ways:
#1) $CC *.cpp  
$CXX *.cpp -o blueconnect-daemon.out

#sudo ./cmake-build-debug/OBD 1> testresult.txt
#user input cmd: 1)AT+DS048,DS049 2)AT+STOPDS 3)AT+DTC 4)quit or QUIT to exit

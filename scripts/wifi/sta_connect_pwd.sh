#example:  ./connect_pwd.sh  Lavida 11111111a
if [ $# -ne 2 ]; then
  echo "usage:./connect_pwd.sh ssid pwd"
  exit 1
else
  echo "ssid=$1 ,pwd=$2 "
fi 

wpa_cli -iwlan0 disconnect
for i in `wpa_cli list_networks | grep ^[0-9] | cut -f1`; do wpa_cli -iwlan0 remove_network $i; done
wpa_cli -iwlan0 add_network
wpa_cli -iwlan0 set_network 0 auth_alg OPEN
wpa_cli -iwlan0 set_network 0 key_mgmt WPA-PSK
wpa_cli -iwlan0 set_network 0 psk "\"$2\"" 
wpa_cli -iwlan0 set_network 0 mode 0
wpa_cli -iwlan0 set_network 0 ssid "\"$1\"" 
wpa_cli -iwlan0 select_network 0
wpa_cli -iwlan0 enable_network 0
wpa_cli -iwlan0 reassociate
udhcpc -i wlan0
sleep 3
wpa_cli -iwlan0 status
iw wlan0 link

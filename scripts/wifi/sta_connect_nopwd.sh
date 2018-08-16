#remebmer to excute follow cmd after system reboot,otherwise will have error(Failed to connect to non-global ctrl_ifname: wlan0  error: No such file or directory)
#wpa_supplicant -d -Dnl80211 -c/etc/wpa_supplicant.conf -iwlan0 -B

if [ $# -ne 1 ]; then
  echo "usage:./connect_nopwd.sh ssid"
  exit 1
fi 
wpa_cli -iwlan0 disconnect
for i in `wpa_cli -iwlan0 list_networks | grep ^[0-9] | cut -f1`; do wpa_cli -iwlan0 remove_network $i; done
wpa_cli -iwlan0 add_network
wpa_cli -iwlan0 set_network 0 auth_alg OPEN
wpa_cli -iwlan0 set_network 0 key_mgmt NONE
wpa_cli -iwlan0 set_network 0 mode 0
wpa_cli -iwlan0 set_network 0 ssid $1
wpa_cli -iwlan0 select_network 0
wpa_cli -iwlan0 enable_network 0
wpa_cli -iwlan0 reassociate
udhcpc -i wlan0  #obtain ip address
sleep 3
wpa_cli -iwlan0 status

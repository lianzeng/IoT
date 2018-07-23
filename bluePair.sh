#!/usr/bin/expect -f

#usage: ./bluetoothctl_auto.sh 00:1D:A5:1D:0A:49

set prompt "#"
set address [lindex $argv 0]

spawn  bluetoothctl -a 

#### below 3 line may not need, remove may cause device not found
#expect -re $prompt
#send "remove $address\r"
#sleep 1
####

expect -re $prompt
send "scan on\r"

#send_user "\nscan on\r"
sleep 10

send "scan off\r"
expect "Controller"  


send "trust $address\r"  
sleep 2

send "pair $address\r"
expect {
"PIN code" {
 send "1234\r"
 }
"not available"
}


sleep 3

send "quit\r"
expect eof
 


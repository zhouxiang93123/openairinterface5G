echo $UE_IP
timeout -s 9 20s iperf -c $UE_IP -i1

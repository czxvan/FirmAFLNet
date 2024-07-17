# service forward
iptables -t nat -A PREROUTING -p tcp --dport 80 -j DNAT --to-destination 192.168.0.1:80
iptables -t nat -A POSTROUTING -p tcp -d 192.168.0.1 --dport 80 -j MASQUERADE
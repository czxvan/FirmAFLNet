TARGET_NAME=httpd
# IP=$(cat ip)
IP=192.168.0.1

curl $IP --max-time 2 \
    >& /dev/null
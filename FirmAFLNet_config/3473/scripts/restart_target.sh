TARGET_NAME=httpd
# IP=$(cat ip)
IP=192.168.0.1
CMD="pids=\`ps | grep httpd | grep -v grep | sed 's/^ *//' | cut -d ' ' -f 1\` && for pid in \$pids; do kill -9 \$pid; done; \
    httpd -f /var/run/httpd.conf"

curl $IP:4817/execute \
    -d "{\"cmd\": \"$CMD\"}" \
    >& /dev/null
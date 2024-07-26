TARGET_NAME=httpd
# IP=$(cat ip)
IP=192.168.1.1
CMD="pids=\`ps | grep httpd | grep -v grep | sed 's/^ *//' | cut -d ' ' -f 1\` && for pid in \$pids; do kill -9 \$pid; done; \
    httpd -S -E /usr/sbin/ca.pem /usr/sbin/httpsd.pem"

curl $IP:4817/execute \
    -d "{\"cmd\": \"$CMD\"}" \
    >& /dev/null
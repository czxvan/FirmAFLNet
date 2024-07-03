KERN=/home/czx/openbmc-workspace/openbmc/build/romulus/tmp/deploy/images/romulus
# sudo gdbserver 127.0.0.1:1234 \
    ../../build/qemu-system-arm \
    -m 1G -machine romulus-bmc \
    -drive file=$KERN/obmc-phosphor-image-romulus.static.mtd,if=mtd,format=raw\
    -net nic \
    -net user,hostfwd=:127.0.0.1:18080-:8080,hostfwd=:127.0.0.1:14817-:4817,hostname=qemu \
    -D qemu_log.txt \
    -nographic \
    -d plugin \
    -plugin ../build/libaflspy.so \




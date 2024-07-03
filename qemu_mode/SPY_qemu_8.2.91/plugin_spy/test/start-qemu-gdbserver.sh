KERN=/home/czx/openbmc-workspace/openbmc/build/romulus/tmp/deploy/images/romulus
# sudo ../../build/qemu-system-arm \
#     -m 256 -machine romulus-bmc \
#     -drive file=$KERN/obmc-phosphor-image-romulus.static.mtd,if=mtd,format=raw\
#     -net nic \
#     -net user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:280-:80,hostfwd=:127.0.0.1:2443-:443,hostfwd=:127.0.0.1:18080-:8080,hostfwd=udp:127.0.0.1:2623-:623,hostname=qemu \
#     -d plugin \
#     -plugin ../build/libaflspy.so \
#     -D qemu_log.txt \
#     -nographic

sudo gdbserver 127.0.0.1:1234 ../../build/qemu-system-arm \
    -m 256 -machine romulus-bmc \
    -drive file=$KERN/obmc-phosphor-image-romulus.static.mtd,if=mtd,format=raw\
    -net nic \
    -net user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:280-:80,hostfwd=:127.0.0.1:2443-:443,hostfwd=:127.0.0.1:18080-:8080,hostfwd=udp:127.0.0.1:2623-:623,hostname=qemu \
    -d plugin \
    -D qemu_log.txt \
    -nographic

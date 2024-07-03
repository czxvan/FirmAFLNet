export QEMU_MODE=USER
../../build/qemu-arm \
    -d plugin \
    -plugin ../build/libaflspy.so \
    -D qemu_log.txt \
    ../build/hello-syscall
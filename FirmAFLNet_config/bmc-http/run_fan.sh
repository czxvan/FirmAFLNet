KERN=/workspaces/firmaflnet-repro/FirmAFLNet/firmware/bmc
QEMUSPY=/workspaces/firmaflnet-repro/FirmAFLNet/qemu_mode/SPY_qemu_8.2.91
AFLSPY=$QEMUSPY/plugin_spy
export AFL_FAST_CAL=1
# gdbserver 127.0.0.1:1234 \
../afl-fuzz -i ./inputs -o ./outputs -P HTTP -N tcp://127.0.0.1/18080 -m 4096M -QQ \
     -d -q 3 -s 3 -E -R -W 5 -w 50000 -t 50000 \
     -- \
     $QEMUSPY/build/qemu-system-arm \
    -m 256 -machine romulus-bmc \
    -drive file=$KERN/obmc-phosphor-image-romulus.static.mtd,if=mtd,format=raw\
    -net nic \
    -net user,hostfwd=:127.0.0.1:18080-:8080,hostfwd=:127.0.0.1:24817-:4817,hostname=qemu \
    -d plugin \
    -plugin $AFLSPY/build/libaflspy-arm.so \
    -D qemu_log.txt \
    -nographic
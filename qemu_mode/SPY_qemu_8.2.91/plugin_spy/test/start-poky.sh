KERN=/home/czx/openbmc-workspace/poky/build/tmp/deploy/images/qemux86
qemu-system-i386 \
    -drive file=$KERN/core-image-minimal-qemux86.rootfs.ext4,if=virtio,format=raw \
    -usb -device usb-tablet -usb -device usb-kbd \
    -cpu IvyBridge -machine q35 -smp 4 -m 256 \
    -kernel $KERN/bzImage \
    -append 'root=/dev/vda rw' \
    -nographic
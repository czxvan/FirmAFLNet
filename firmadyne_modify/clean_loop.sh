if [ $# -lt 1 ]; then
    echo "Usage: clean_loop.sh <loop ID>"
    echo "Example: clean_loop.sh 3"
    exit 1
fi

kpartx -vd /dev/loop$1
losetup -vd /dev/loop$1
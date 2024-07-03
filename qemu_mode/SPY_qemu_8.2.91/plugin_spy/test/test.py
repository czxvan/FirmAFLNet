import os

KERN="/home/czx/openbmc-workspace/openbmc/build/romulus/tmp/deploy/images/romulus"
os.execve('/usr/bin/gdbserver', 
            ['/usr/bin/gdbserver', '127.0.0.1:1234',
            '/home/czx/qemu-workspace/qemu-spy/build/qemu-system-arm',
            '-m', '256M', '-M', 'romulus-bmc',
            '-drive', f'file={KERN}/obmc-phosphor-image-romulus.static.mtd,if=mtd,format=raw',
            '-net', 'nic',
            '-net', 'user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:280-:80,hostfwd=:127.0.0.1:2443-:443,hostfwd=:127.0.0.1:18080-:8080,hostfwd=udp:127.0.0.1:2623-:623,hostname=qemu',
            '-d', 'plugin',
            '-plugin', '../build/libaflspy.so',
            '-D', 'qemu_log.txt',
            '-nographic'], {})
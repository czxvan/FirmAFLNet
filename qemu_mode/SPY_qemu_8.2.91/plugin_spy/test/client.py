from math import fabs
import os
import time
import signal
CTL_READ_FD = 198
STATE_WRITE_FD = 199
DEBUG = False
if __name__ == '__main__':
    ctl_rfd, ctl_wfd = os.pipe()
    state_rfd, state_wfd = os.pipe()
    pid = os.fork()
    if pid == 0:
        os.dup2(ctl_rfd, CTL_READ_FD)
        os.dup2(state_wfd, STATE_WRITE_FD)
        os.close(ctl_rfd)
        os.close(ctl_wfd)
        os.close(state_rfd)
        os.close(state_wfd)
        KERN="/home/czx/openbmc-workspace/openbmc/build/romulus/tmp/deploy/images/romulus"
        if DEBUG:
            os.execve('/usr/bin/gdbserver', 
                    ['/usr/bin/gdbserver', '127.0.0.1:1234',
                        '/home/czx/qemu-workspace/qemu-spy/build/qemu-system-arm',
                        '-m', '256M', '-M', 'romulus-bmc',
                        '-drive', f'file={KERN}/obmc-phosphor-image-romulus.static.mtd,if=mtd,format=raw',
                        '-net', 'nic',
                        '-net', 'user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:2443-:443,hostfwd=:127.0.0.1:18080-:8080,hostfwd=udp:127.0.0.1:2623-:623,hostname=qemu',
                        '-d', 'plugin',
                        '-plugin', '../build/libaflspy.so',
                        '-D', 'qemu_log.txt',
                        '-nographic'], {})
        else:
            os.execve('../../build/qemu-system-arm', 
                    ['qemu-system-arm',
                        '-m', '256M', '-M', 'romulus-bmc',
                        '-drive', f'file={KERN}/obmc-phosphor-image-romulus.static.mtd,if=mtd,format=raw',
                        '-net', 'nic',
                        '-net', 'user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:2443-:443,hostfwd=:127.0.0.1:18080-:8080,hostfwd=udp:127.0.0.1:2623-:623,hostname=qemu',
                        '-d', 'plugin',
                        '-plugin', '../build/libaflspy.so',
                        '-D', 'qemu_log.txt',
                        '-nographic'], {})
    else:
        try:
            os.close(ctl_rfd)
            os.close(state_wfd)

            s = os.read(state_rfd, 4)
            if s == b'RDY!':
                print("read: ", s)
                os.write(ctl_wfd, b'FORK')
                print("write: FORK")
            else:
                print("子进程状态错误")

            while True:
                time.sleep(1)
                os.write(ctl_wfd, b'STRT')
                print("write: STRT")
                # do test here
                time.sleep(1)
                os.write(ctl_wfd, b'DONE')
                print("write: DONE")
                # 等待测试结果
                s = os.read(state_rfd, 4)
                print("State: ", s)
            # os.waitpid(pid, 0)
        except KeyboardInterrupt:
            print("父进程被中断")
            # 杀掉子进程
            os.kill(pid, signal.SIGTERM)
            time.sleep(1)  # 等待子进程退出
            os._exit(0)  # 退出父进程
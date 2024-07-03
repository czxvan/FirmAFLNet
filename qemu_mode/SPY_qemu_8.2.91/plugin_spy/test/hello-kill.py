import os
import time
import signal

def child_process():
    print("子进程开始执行")
    try:
        while True:
            time.sleep(1)
            print("子进程正在运行")
    except KeyboardInterrupt:
        print("子进程被中断")

def main():
    pid = os.fork()
    if pid == 0:
        # 子进程
        child_process()
    else:
        # 父进程
        print("父进程开始执行")
        try:
            while True:
                print("父进程正在运行")
                time.sleep(1)
        except KeyboardInterrupt:
            print("父进程被中断")
            # 杀掉子进程
            os.kill(pid, signal.SIGTERM)
            time.sleep(1)  # 等待子进程退出
            os._exit(0)  # 退出父进程

if __name__ == "__main__":
    main()
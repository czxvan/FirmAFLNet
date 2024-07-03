#include <stdio.h>
#include <unistd.h>

const int CTL_READ_FD = 198;
const int STATE_WRITE_FD = 199;
int main() {
    
    while (1) {
        char buf[1024];
        int n = read(CTL_READ_FD, buf, sizeof(buf));
        
        printf("read %d bytes: %s\n", n, buf);
        
        if (n == 4)
            break;
        write(STATE_WRITE_FD, buf, n);
    }

    return 0;
}
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void) {
    int ret = fork();
    if (ret == 0) {
        printf("hello from child\n");
    } else {
        wait(NULL);
        printf("hello from parent\n");
    }
    return 0;
}
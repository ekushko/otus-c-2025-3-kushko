/*
=== STEP-BY-STEP INSTRUCTIONS ===

TERMINAL 1 (User terminal):
1. ./ftrace_demo
   # Program shows PID and waits for Enter
   # COPY THE PID NUMBER - you need it for terminal 2!

TERMINAL 2 (Root terminal - run these commands):
2. sudo -i
3. cd /sys/kernel/debug/tracing
4. echo 0 > tracing_on                    # Stop any existing trace
5. echo > trace                           # Clear trace buffer
6. echo function > current_tracer         # Use function tracer (most reliable)
7. echo '__x64_sys_*' > set_ftrace_filter # Filter only syscalls
8. echo YOUR_PID_HERE > set_ftrace_pid    # Replace with PID from terminal 1
9. echo 1 > tracing_on                    # Start tracing

TERMINAL 1 (back to user terminal):
10. Press Enter in the program
    # Program executes and shows syscall operations

TERMINAL 2 (back to root terminal):
11. echo 0 > tracing_on                   # Stop tracing
12. cat trace > /tmp/ftrace_output.txt    # Save results
13. echo > set_ftrace_pid                 # Clean up PID filter
14. less /tmp/ftrace_output.txt           # View results

TROUBLESHOOTING:
- If trace is empty: check 'cat available_filter_functions | grep sys_'
- For ARM64: use '__arm64_sys_*' instead of '__x64_sys_*'
- Must run ftrace commands as root!
- Timing is critical - set PID filter BEFORE pressing Enter!
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

int main() {
    printf("=== FTRACE SYSCALL DEMO ===\n");
    printf("PID: %d\n", getpid());
    printf("\nCOPY THE PID NUMBER ABOVE!\n");
    printf("Set up ftrace in another terminal, then press Enter...\n");
    printf("Waiting for Enter: ");
    fflush(stdout);

    getchar();  // Wait for user to set up ftrace

    printf("\n=== SYSCALL OPERATIONS START ===\n");

    // SYSCALL 1: openat() - open/create file
    printf("1. Opening file (openat syscall)...\n");
    int fd = open("/tmp/test_ftrace.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open failed");
        exit(1);
    }
    printf("   File opened, fd = %d\n", fd);

    // SYSCALL 2: write() - write data to file
    printf("2. Writing to file (write syscall)...\n");
    const char *message = "Hello from ftrace syscall demo!\n";
    ssize_t bytes_written = write(fd, message, strlen(message));
    printf("   Written %ld bytes\n", bytes_written);

    // SYSCALL 3: newfstat() - get file information
    printf("3. Getting file info (newfstat syscall)...\n");
    struct stat file_stat;
    if (fstat(fd, &file_stat) == 0) {
        printf("   File size: %ld bytes\n", file_stat.st_size);
        printf("   File permissions: %o\n", file_stat.st_mode & 0777);
    }

    // SYSCALL 4: close() - close file descriptor
    printf("4. Closing file (close syscall)...\n");
    close(fd);
    printf("   File closed\n");

    // SYSCALL 5: unlink() - delete file
    printf("5. Deleting file (unlink syscall)...\n");
    if (unlink("/tmp/test_ftrace.txt") == 0) {
        printf("   File deleted\n");
    } else {
        perror("unlink failed");
    }

    // SYSCALL 6: chdir() - changing directory
    printf("6. Change directory (chdir syscall)...\n");
    chdir("/tmp/");

    printf("\n=== SYSCALL OPERATIONS END ===\n");
    printf("Check /tmp/ftrace_output.txt for syscall traces!\n");

    return 0;
}

/*
EXPECTED OUTPUT in /tmp/ftrace_output.txt:
- Lines starting with your PID
- Function calls like __x64_sys_openat, __x64_sys_write, etc.
- Entry and exit points for each syscall
- Timing information for each operation

WHAT YOU'LL LEARN:
- How userspace programs trigger kernel syscalls
- The actual kernel function names for syscalls
- Timing and execution flow of system calls
- How ftrace can monitor kernel execution in real-time
*/

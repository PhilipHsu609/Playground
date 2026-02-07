#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sched.h>
#include <seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <linux/capability.h>
#include <linux/limits.h>

struct child_config {
    int argc;
    uid_t uid;
    int fd;
    char *hostname;
    char **argv;
    char *mount_dir;
};

// capabilities
int capabilities() {
    fprintf(stderr, "=> dropping capabilities...");

    int drop_caps[] = {
        // Access to the audit system
        CAP_AUDIT_CONTROL,
        CAP_AUDIT_READ,
        CAP_AUDIT_WRITE,
        // Employ features that can block system suspend
        CAP_BLOCK_SUSPEND,
        // Bypass file read permission checks
        CAP_DAC_READ_SEARCH,
        // Disallow modifying a setuid executable
        CAP_FSETID,
        // Lock memory
        CAP_IPC_LOCK,
        // Allow or override MAC (Mandatory Access Control) policies
        CAP_MAC_ADMIN,
        CAP_MAC_OVERRIDE,
        // Create device files
        CAP_MKNOD,
        // Set arbitrary capabilities on a file
        CAP_SETFCAP,
        // Perform privileged syslog operations
        CAP_SYSLOG,
        // Admin privileges for system configuration
        CAP_SYS_ADMIN,
        // Restart system
        CAP_SYS_BOOT,
        // Modify kernel parameters
        CAP_SYS_MODULE,
        // Adjust process nice values
        CAP_SYS_NICE,
        // Perform raw I/O operations
        CAP_SYS_RAWIO,
        // Override resource limits
        CAP_SYS_RESOURCE,
        // Modify system time
        CAP_SYS_TIME,
        // Trigger something that will wake up the system
        CAP_WAKE_ALARM,
    };

    size_t num_caps = sizeof(drop_caps) / sizeof(*drop_caps);

    fprintf(stderr, "bounding...");
    for (size_t i = 0; i < num_caps; i++) {
        if (prctl(PR_CAPBSET_DROP, drop_caps[i], 0, 0, 0)) {
            fprintf(stderr, "failed to drop capability %d: %m\n", drop_caps[i]);
            return -1;
        }
    }

    fprintf(stderr, "inhereitable...");
    cap_t caps = NULL;
    if (!(caps = cap_get_proc()) ||
        cap_set_flag(caps, CAP_INHERITABLE, num_caps, drop_caps, CAP_CLEAR) ||
        cap_set_proc(caps)) {
        fprintf(stderr, "failed to drop inheritable capabilities: %m\n");
        if (caps) {
            cap_free(caps);
        }
        return -1;
    }

    cap_free(caps);
    fprintf(stderr, "done.\n");
    return 0;
}

// mounts
int pivot_root(const char *new_root, const char *put_old) {
    // pivot_root syscall is not exposed in glibc, so we use syscall directly
    return syscall(SYS_pivot_root, new_root, put_old);
}

int mounts(struct child_config *config) {
    fprintf(stderr, "=> remounting everything with MS_PRIVATE...");
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL)) {
        fprintf(stderr, "mount(MS_PRIVATE) failed: %m\n");
        return -1;
    }
    fprintf(stderr, "remounted.\n");

    fprintf(stderr, "=> making a temp directory and a bind mount there...");
    char mount_dir[] = "/tmp/tmp.XXXXXX";
    if (!mkdtemp(mount_dir)) {
        fprintf(stderr, "mkdtemp() failed: %m\n");
        return -1;
    }

    // Bind mount the the container's rootfs to the temp directory
    if (mount(config->mount_dir, mount_dir, NULL, MS_BIND | MS_PRIVATE, NULL)) {
        fprintf(stderr, "mount(MS_BIND) failed: %m\n");
        return -1;
    }

    // Create a temporary directory inside the new root for pivot_root.
    // We will pivot the old root to this directory.
    char inner_mount_dir[] = "/tmp/tmp.XXXXXX/oldroot.XXXXXX";
    memcpy(inner_mount_dir, mount_dir, sizeof(mount_dir) - 1);
    if (!mkdtemp(inner_mount_dir)) {
        fprintf(stderr, "mkdtemp() failed: %m\n");
        return -1;
    }
    fprintf(stderr, "done.\n");

    //* Why we need pivot_root:
    //* pivot_root changes the root filesystem of the calling process to that
    //* specified by new_root, and puts the old root at the location specified
    //* by put_old. This is essential for isolating the process in a new rootfs.
    //*
    //* Why keep a directory for the old root:
    //* The pivot_root syscall requires a directory to place the old root.
    //* This directory must be inside the new root filesystem.
    //* Or simply, we can't unmount the old root we are currently using.

    // Before pivoting, the child sees the same filesystem as the parent.

    fprintf(stderr, "=> pivoting root...");
    if (pivot_root(mount_dir, inner_mount_dir)) {
        fprintf(stderr, "pivot_root() failed: %m\n");
        return -1;
    }
    fprintf(stderr, "done.\n");

    // After pivoting, the child sees the new rootfs (mount_dir) as its root.

    char *old_root_dir = basename(inner_mount_dir);
    char old_root[sizeof(inner_mount_dir) + 1] = {'/'};
    strcpy(old_root + 1, old_root_dir); // "/oldroot.XXXXXX"

    fprintf(stderr, "=> unmounting %s...", old_root);

    // Change working directory to the new root
    if (chdir("/")) {
        fprintf(stderr, "chdir(/) failed: %m\n");
        return -1;
    }
    // Unmount the old root which is now at /oldroot.XXXXXX
    if (umount2(old_root, MNT_DETACH)) {
        fprintf(stderr, "umount2(%s) failed: %m\n", old_root);
        return -1;
    }
    if (rmdir(old_root)) {
        fprintf(stderr, "rmdir(%s) failed: %m\n", old_root);
        return -1;
    }

    fprintf(stderr, "done.\n");
    return 0;
}

// sycalls
#define SCMP_FAIL SCMP_ACT_ERRNO(EPERM)

int syscalls() {
    scmp_filter_ctx ctx = NULL;
    fprintf(stderr, "=> filtering syscalls...");
    // SCMP_ACT_ALLOW: Allow the syscalls by default
    if (!(ctx = seccomp_init(SCMP_ACT_ALLOW))
        // Block chmod/fchmod/fchmodat when setting setuid or setgid bits
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(chmod), 1,
                            SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(chmod), 1,
                         SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmod), 1,
                         SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmod), 1,
                         SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmodat), 1,
                         SCMP_A2(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmodat), 1,
                         SCMP_A2(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID))
        // Block unshare when trying to create a new user namespace
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(unshare), 1,
                            SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER)) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(clone), 1,
                         SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER))
        // Block process to write to the controlling terminal
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(ioctl), 1,
                            SCMP_A1(SCMP_CMP_MASKED_EQ, TIOCSTI, TIOCSTI))
        // Block the kernel keyring syscalls
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(keyctl), 0) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(add_key), 0) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(request_key), 0)
        // Before Linux 4.8, ptrace breaks seccomp
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(ptrace), 0)
        // Block NUMA-related syscalls
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(mbind), 0) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(migrate_pages), 0) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(move_pages), 0) ||
        seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(set_mempolicy), 0)
        // Block userspace page fault handling
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(userfaultfd), 0)
        // Prevent reading the information on the host with perf events
        || seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(perf_event_open), 0)
        // ?
        || seccomp_attr_set(ctx, SCMP_FLTATR_CTL_NNP, 0) || seccomp_load(ctx)) {
        if (ctx) {
            seccomp_release(ctx);
            fprintf(stderr, "failed: %m\n");
            return 1;
        }
    }
    seccomp_release(ctx);
    fprintf(stderr, "done.\n");
    return 0;
}

// resources
#define MEMORY "1073741824" // 1GB
#define SHARES "256"        // CPU shares
#define PIDS "64"           // Max processes
#define WEIGHT "10"         // IO weight
#define FD_COUNT 64         // Max file descriptors

struct cgrp_setting {
    char name[256];
    char value[256];
};

// cgroup v2 controllers and their settings
struct cgrp_setting *cgrp_settings[] = {
    &(struct cgrp_setting){
        .name = "memory.max",
        .value = MEMORY,
    },
    &(struct cgrp_setting) {
        .name = "cpu.weight",
        .value = SHARES,
    },
    &(struct cgrp_setting) {
        .name = "pids.max",
        .value = PIDS,
    },
    &(struct cgrp_setting) {
        .name = "io.weight",
        .value = WEIGHT,
    },
    &(struct cgrp_setting){
        .name = "cgroup.procs",
        .value = "0",
    },
    NULL
};

// Enable the controllers in the root cgroup so that we can use them in child cgroups
int cgrp_enable_controllers(void) {
    int fd = open("/sys/fs/cgroup/cgroup.subtree_control", O_WRONLY);
    if (fd == -1) {
        fprintf(stderr, "open(cgroup.subtree_control) failed: %m\n");
        return -1;
    }
    // Enable memory, cpu, pids, and io controllers in the root cgroup so that we can use them in child cgroups
    if (dprintf(fd, "+memory +cpu +pids +io") == -1) {
        fprintf(stderr, "dprintf() failed: %m\n");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int resources(struct child_config *config) {
    fprintf(stderr, "=> setting cgroups...");
    // Enable the controllers in the root cgroup
    if (cgrp_enable_controllers()) {
        return -1;
    }
    
    char dir[PATH_MAX] = {0};
    // Create a new cgroup directory for the child process
    if (snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s", config->hostname) == -1) {
        fprintf(stderr, "path too long\n");
        return -1;
    }
    if (mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR)) {
        fprintf(stderr, "mkdir(%s) failed: %m\n", dir);
        return -1;
    }
    
    // Write the resource limits to the cgroup's control files
    for (struct cgrp_setting **setting = cgrp_settings; *setting; setting++) {
        char path[PATH_MAX] = {0};
        if (snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/%s", config->hostname,
                     (*setting)->name) == -1) {
            fprintf(stderr, "path too long\n");
            return -1;
        }
        int fd = open(path, O_WRONLY);
        if (fd == -1) {
            fprintf(stderr, "open(%s) failed: %m\n", path);
            return -1;
        }
        if (dprintf(fd, "%s", (*setting)->value) == -1) {
            fprintf(stderr, "dprintf(%s) failed: %m\n", path);
            close(fd);
            return -1;
        }
        close(fd);
    }
    fprintf(stderr, "done.\n");

    fprintf(stderr, "=> setting rlimits...");
    if (setrlimit(RLIMIT_NOFILE,
                  &(struct rlimit){.rlim_cur = FD_COUNT, .rlim_max = FD_COUNT})) {
        fprintf(stderr, "setrlimit(RLIMIT_NOFILE) failed: %m\n");
        return 1;
    }
    fprintf(stderr, "done.\n");
    return 0;
}

int free_resources(struct child_config *config) {
    fprintf(stderr, "=> cleaning cgroups...");
    
    // Move ourselves back to the root cgroup
    char procs_path[] = "/sys/fs/cgroup/cgroup.procs";
    int procs_fd = open(procs_path, O_WRONLY);
    if(procs_fd == -1) {
        fprintf(stderr, "open(%s) failed: %m\n", procs_path);
        return -1;
    }
    if(dprintf(procs_fd, "%d", 0) == -1) {
        fprintf(stderr, "dprintf(%s) failed: %m\n", procs_path);
        close(procs_fd);
        return -1;
    }
    close(procs_fd);

    // Remove the cgroup directory for the child process
    char cgrp_path[PATH_MAX] = {0};
    if (snprintf(cgrp_path, sizeof(cgrp_path), "/sys/fs/cgroup/%s", config->hostname) == -1) {
        fprintf(stderr, "path too long\n");
        return -1;
    }
    if (rmdir(cgrp_path)) {
        fprintf(stderr, "rmdir(%s) failed: %m\n", cgrp_path);
        return -1;
    }

    fprintf(stderr, "done.\n");
    return 0;
}

// child's user namespace
#define USERNS_OFFSET 10000
#define USERNS_COUNT 2000

int handle_child_uid_map(pid_t child_pid, int fd) {
    int uid_map = 0;
    int has_userns = -1;
    // Check if the child created a user namespace
    if (read(fd, &has_userns, sizeof(has_userns)) != sizeof(has_userns)) {
        fprintf(stderr, "could not read from child: %m\n");
        return -1;
    }
    if (has_userns) {
        char path[PATH_MAX] = {0};
        for (char **file = (char *[]){"uid_map", "gid_map", 0}; *file; file++) {
            // Construct the path to the child's uid_map/gid_map
            // e.g., /proc/<child_pid>/uid_map, /proc/<child_pid>/gid_map
            if ((size_t)snprintf(path, sizeof(path), "/proc/%d/%s", child_pid, *file) >
                sizeof(path)) {
                fprintf(stderr, "path too long\n");
                return -1;
            }

            fprintf(stderr, "=> writing %s...\n", path);

            if ((uid_map = open(path, O_WRONLY)) == -1) {
                fprintf(stderr, "open(%s) failed: %m\n", path);
                return -1;
            }

            // Write the mapping: "0 <host_uid> <count>\n"
            // Map UID 0 in the namespace to <host_uid> on the host and up to <count> UIDs
            if (dprintf(uid_map, "0 %d %d\n", USERNS_OFFSET, USERNS_COUNT) == -1) {
                fprintf(stderr, "dprintf(%s) failed: %m\n", path);
                close(uid_map);
                return -1;
            }
            close(uid_map);
        }
    }

    // Notify the child that the mapping is done
    if (write(fd, &(int){0}, sizeof(int)) != sizeof(int)) {
        fprintf(stderr, "could not write to child: %m\n");
        return -1;
    }
    return 0;
}

int userns(struct child_config *config) {
    fprintf(stderr, "=> trying a user namespace...");

    // Attempt to unshare the user namespace.
    // The child process is running in the same namespace as the parent, so this will try
    // to create a user namespace if supported.
    int has_userns = !unshare(CLONE_NEWUSER);

    // Notify the parent about the result
    if (write(config->fd, &has_userns, sizeof(has_userns)) != sizeof(has_userns)) {
        fprintf(stderr, "could not write to parent: %m\n");
        return -1;
    }

    // Wait for the parent to set up the UID/GID mappings
    int result = 0;
    if (read(config->fd, &result, sizeof(result)) != sizeof(result)) {
        fprintf(stderr, "could not read from parent: %m\n");
        return -1;
    }

    if (result) {
        return -1;
    }

    if (has_userns) {
        fprintf(stderr, "created.\n");
    } else {
        fprintf(stderr, "not supported.\n");
    }

    fprintf(stderr, "=> switching to uid %d / gid %d...", config->uid, config->uid);

    // Switch to the specified UID/GID inside the user namespace
    if (setgroups(
            1,
            &(gid_t){
                config->uid}) || // Override supplementary groups with just the target GID
        setresgid(config->uid, config->uid, config->uid) ||
        setresuid(config->uid, config->uid, config->uid)) {
        fprintf(stderr, "failed: %m\n");
        return -1;
    }

    fprintf(stderr, "switched.\n");
    return 0;
}

int child(void *arg) {
    struct child_config *config = arg;
    if (sethostname(config->hostname, strlen(config->hostname)) ||
        mounts(config)    // mounts
        || userns(config) // user namespace
        || capabilities() // capabilities
        || syscalls()) {  // syscall filtering
        close(config->fd);
        return -1;
    }

    if (close(config->fd)) {
        fprintf(stderr, "close() failed: %m\n");
        return -1;
    }
    if (execve(config->argv[0], config->argv, NULL)) {
        fprintf(stderr, "execve(%s) failed: %m\n", config->argv[0]);
        return -1;
    }

    return 0;
}

int choose_hostname(char *buff, size_t len) {
    static const char *suits[] = {"swords", "wands", "pentacles", "cups"};
    static const char *minor[] = {"ace",  "two",    "three", "four", "five",
                                  "six",  "seven",  "eight", "nine", "ten",
                                  "page", "knight", "queen", "king"};
    static const char *major[] = {
        "fool",       "magician", "high-priestess", "empress", "emperor", "hierophant",
        "lovers",     "chariot",  "strength",       "hermit",  "wheel",   "justice",
        "hanged-man", "death",    "temperance",     "devil",   "tower",   "star",
        "moon",       "sun",      "judgment",       "world"};
    struct timespec now = {0};
    clock_gettime(CLOCK_MONOTONIC, &now);
    size_t ix = now.tv_nsec % 78;
    if (ix < sizeof(major) / sizeof(*major)) {
        snprintf(buff, len, "%05lx-%s", now.tv_sec, major[ix]);
    } else {
        ix -= sizeof(major) / sizeof(*major);
        snprintf(buff, len, "%05lxc-%s-of-%s", now.tv_sec,
                 minor[ix % (sizeof(minor) / sizeof(*minor))],
                 suits[ix / (sizeof(minor) / sizeof(*minor))]);
    }
    return 0;
}

int main(int argc, char **argv) {
    struct child_config config = {0};
    int err = 0;
    int option = 0;
    int sockets[2] = {0};
    pid_t child_pid = 0;
    int last_optind = 0;

    // Usage: ./contained -m ./rootfs -u 0 -c /bin/sh
    while ((option = getopt(argc, argv, "c:m:u:"))) {
        switch (option) {
        case 'c':
            config.argc = argc - last_optind - 1;
            config.argv = &argv[argc - config.argc];
            goto finish_options;
        case 'm':
            config.mount_dir = optarg;
            break;
        case 'u':
            if (sscanf(optarg, "%d", &config.uid) != 1) {
                fprintf(stderr, "badly-formatted uid: %s\b", optarg);
                goto usage;
            }
            break;
        default:
            goto usage;
        }
        last_optind = optind;
    }

finish_options:
    if (!config.argc) {
        goto usage;
    }
    if (!config.mount_dir) {
        goto usage;
    }

    // check-linux-version
    fprintf(stderr, "=> validating Linux version...");
    struct utsname host = {0};
    if (uname(&host)) {
        fprintf(stderr, "failed %m\n");
        goto cleanup;
    }
    int major = -1;
    int minor = -1;
    if (sscanf(host.release, "%u.%u", &major, &minor) != 2) {
        fprintf(stderr, "weird release format: %s\n", host.release);
        goto cleanup;
    }
    if (strcmp("x86_64", host.machine) != 0) {
        fprintf(stderr, "expected x86_64: %s\n", host.machine);
        goto cleanup;
    }

    fprintf(stderr, "%s on %s.\n", host.release, host.machine);

    char hostname[256] = {0};
    if (choose_hostname(hostname, sizeof(hostname))) {
        goto error;
    }
    config.hostname = hostname;

    // namespaces
    if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets)) {
        fprintf(stderr, "socketpair() failed: %m\n");
        goto error;
    }
    if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC)) {
        fprintf(stderr, "fcntl() failed: %m\n");
        goto error;
    }
    config.fd = sockets[1]; // pass to child

// Setup a room for stack
#define STACK_SIZE (1024 * 1024)
    char *stack = NULL;
    if (!(stack = malloc(STACK_SIZE))) {
        fprintf(stderr, "malloc() failed: %m\n");
        goto error;
    }

    // Prepare the cgroup
    if (resources(&config)) {
        err = 1;
        goto clear_resources;
    }

    // clone syscall flags
    int flags =
        CLONE_NEWNS       // New Mount Namespace (files)
        | CLONE_NEWCGROUP // New Cgroup (resource limits)
        | CLONE_NEWPID    // New PID Namespace (processes)
        | CLONE_NEWIPC    // New IPC Namespace (message queues, semaphores, shared memory)
        | CLONE_NEWNET    // New Network Namespace (network interfaces)
        | CLONE_NEWUTS    // New UTS Namespace (hostname, domain name)
        | SIGCHLD;        // Make sure we get notified on exit

    // The stack grows downwards, so we need to point to the end of the allocated space
    if ((child_pid = clone(child, stack + STACK_SIZE, flags, &config)) == -1) {
        fprintf(stderr, "clone() failed: %m\n");
        err = 1;
        goto clear_resources;
    }

    // Parent closes child's socket
    close(sockets[1]);
    sockets[1] = 0;

    if (handle_child_uid_map(child_pid, sockets[0])) {
        err = 1;
        goto kill_and_finish_child;
    }

    goto finish_child;

kill_and_finish_child:
    if (child_pid) {
        kill(child_pid, SIGKILL);
    }

finish_child:;
    int child_status = 0;
    waitpid(child_pid, &child_status, 0);
    err |= WEXITSTATUS(child_status);

clear_resources:
    free_resources(&config);
    free(stack);

    goto cleanup;

usage:
    fprintf(stderr, "Usage: %s -u -1 -m . -c /bin/sh ~\n", argv[0]);
error:
    err = 1;
cleanup:
    if (sockets[0]) {
        close(sockets[0]);
    }
    if (sockets[1]) {
        close(sockets[1]);
    }

    return err;
}
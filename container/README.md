# contained

A minimal Linux container runtime in a single C file. Implements the same kernel primitives that Docker and LXC use under the hood — namespaces, pivot_root, cgroups, capabilities, seccomp, and user namespace UID remapping.

Based on [Linux containers in 500 lines of code](https://blog.lizzie.io/linux-containers-in-500-loc.html) by Lizzie Dixon.

## What it does

Running `sudo ./contained -m ./rootfs -u 0 -c /bin/sh` creates an isolated process that:

1. **Lives in its own world** — 6 Linux namespaces (mount, cgroup, PID, IPC, network, UTS) isolate the process from the host
2. **Has its own filesystem** — `pivot_root` swaps the root to an Alpine Linux 3.19.1 rootfs, then unmounts the host filesystem entirely
3. **Can't escape via UIDs** — user namespace maps container UID 0 to unprivileged host UID 10000
4. **Has limited powers** — 19 dangerous Linux capabilities are dropped (no raw I/O, no kernel modules, no reboots, etc.)
5. **Can't call dangerous syscalls** — seccomp BPF blocks setuid chmod, user namespace escape, ptrace, terminal injection, and more
6. **Has bounded resources** — cgroup v2 limits to 1 GB memory, 256 CPU shares, 64 PIDs, and 64 file descriptors

## Building

Requires `clang-18`, `libcap-dev`, and `libseccomp-dev` on an x86_64 Linux system.

```bash
make
```

## Running

Requires root (for namespace creation and cgroup setup):

```bash
sudo ./contained -m ./rootfs -u 0 -c /bin/sh
```

**Options:**
- `-m <path>` — path to the root filesystem (the included `rootfs/` is Alpine Linux 3.19.1)
- `-u <uid>` — UID to run as inside the container (0 = root inside the namespace)
- `-c <cmd> [args...]` — command to execute (must be last, everything after is passed as args)

## How it works

```
main()
 │
 ├─ Validate Linux kernel version and x86_64 architecture
 ├─ Parse args, generate tarot-themed hostname
 ├─ Create socketpair for parent↔child coordination
 ├─ Set up cgroup v2 resource limits
 │
 └─ clone() with CLONE_NEWNS | CLONE_NEWCGROUP | CLONE_NEWPID |
    │          CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWUTS
    │
    ├─ Parent                          ├─ Child
    │   read(socket) ← has_userns?     │   sethostname()
    │   write /proc/<pid>/uid_map      │   mounts()     → pivot_root into rootfs
    │   write /proc/<pid>/gid_map      │   userns()     → CLONE_NEWUSER + notify parent
    │   write(socket) → done           │   capabilities() → drop 19 caps
    │   waitpid()                      │   syscalls()   → install seccomp filter
    │   free_resources()               │   execve()     → become /bin/sh
    │
    └───── socketpair(SOCK_SEQPACKET) ─┘
```

The parent and child synchronize through a socketpair: the child tells the parent whether user namespace creation succeeded, the parent writes UID/GID maps into `/proc/<pid>/{uid,gid}_map`, then signals the child to continue.

## Resource limits

| Resource | Limit | Enforced by |
|----------|-------|-------------|
| Memory | 1 GB | cgroup v2 `memory.max` |
| CPU shares | 256 | cgroup v2 `cpu.weight` |
| Max PIDs | 64 | cgroup v2 `pids.max` |
| IO weight | 10 | cgroup v2 `io.weight` |
| File descriptors | 64 | `setrlimit(RLIMIT_NOFILE)` |

## Blocked syscalls (seccomp)

| Category | What's blocked | Why |
|----------|---------------|-----|
| Privilege escalation | `chmod`/`fchmod`/`fchmodat` with setuid/setgid bits | Prevent creating setuid binaries |
| Namespace escape | `unshare(CLONE_NEWUSER)`, `clone(CLONE_NEWUSER)` | Prevent nested user namespace exploits |
| Terminal injection | `ioctl(TIOCSTI)` | Prevent injecting keystrokes into the controlling terminal |
| Kernel keyring | `keyctl`, `add_key`, `request_key` | Prevent access to host kernel keyring |
| Debugging | `ptrace` | Prevent tracing other processes (also pre-4.8 seccomp bypass) |
| NUMA | `mbind`, `migrate_pages`, `move_pages`, `set_mempolicy` | Prevent NUMA memory manipulation |
| Fault handling | `userfaultfd` | Prevent userspace page fault exploits |
| Profiling | `perf_event_open` | Prevent reading host performance data |

## Dropped capabilities

`AUDIT_CONTROL`, `AUDIT_READ`, `AUDIT_WRITE`, `BLOCK_SUSPEND`, `DAC_READ_SEARCH`, `FSETID`, `IPC_LOCK`, `MAC_ADMIN`, `MAC_OVERRIDE`, `MKNOD`, `SETFCAP`, `SYSLOG`, `SYS_ADMIN`, `SYS_BOOT`, `SYS_MODULE`, `SYS_NICE`, `SYS_RAWIO`, `SYS_RESOURCE`, `SYS_TIME`, `WAKE_ALARM`

## The rootfs

The included `rootfs/` is a minimal Alpine Linux 3.19.1 (x86_64) filesystem with BusyBox, musl libc, OpenSSL 3, and the apk package manager. It provides a functional shell environment inside the container.

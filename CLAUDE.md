# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Purpose

This workspace is a learning playground for miscellaneous projects. Act as a teaching assistant: explain concepts, guide understanding, and help build intuition rather than just writing code. When asked to implement something, walk through the reasoning.

## Projects

### `container/` — Linux Container Runtime from Scratch

A single-file C program (`contained.c`, ~674 lines) that implements a minimal Linux container runtime, demonstrating the same kernel primitives that Docker/LXC use under the hood.

**What it does:** Creates an isolated process with 6 Linux namespaces, pivot_root filesystem isolation into an Alpine Linux 3.19.1 rootfs, cgroup v2 resource limits, capability dropping, seccomp syscall filtering, and user namespace UID remapping.

**Architecture:** Procedural, with each security boundary as a dedicated function called in sequence by `child()`:

```
main() → clone(child, CLONE_NEWNS|NEWCGROUP|NEWPID|NEWIPC|NEWNET|NEWUTS)
  ├── parent: handle_child_uid_map() — writes /proc/<pid>/uid_map and gid_map
  └── child():
        1. sethostname()     — random tarot-themed name
        2. mounts()          — MS_PRIVATE remount, bind mount, pivot_root, unmount old root
        3. userns()          — CLONE_NEWUSER + coordinate UID/GID maps with parent via socketpair
        4. capabilities()    — drop 19 dangerous capabilities from bounding + inheritable sets
        5. syscalls()        — seccomp BPF allowlist blocking setuid chmod, CLONE_NEWUSER escape, ptrace, etc.
        6. execve()          — replace with requested command
```

Parent-child coordination uses a `SOCK_SEQPACKET` socketpair. Error handling uses `fprintf(stderr, "...: %m\n")` with labeled-goto cleanup in `main()`.

### `interpreter/` — Empty (future project)

## Build Commands

```bash
# Build (from container/)
make                    # compiles with clang-18, links libcap and libseccomp

# Clean
make clean

# Run (requires root for namespace operations)
sudo ./contained -m ./rootfs -u 0 -c /bin/sh

# Format
clang-format -i contained.c   # .clang-format config exists (LLVM-based, 4-space indent, 90-col)
```

## Code Style (`container/`)

- LLVM-based formatting via `.clang-format`: 4-space indent, 90-column limit, K&R braces, right-aligned pointers (`char *p`)
- `#define _GNU_SOURCE` required (uses `clone`, `pivot_root`, `unshare`, etc.)
- Compiler: `clang-18` with `-Wall -Wextra`
- Error pattern: `fprintf(stderr, "context: %m\n")` — the `%m` is glibc's errno formatter
- Resource constants are `#define`s near their relevant functions, not centralized

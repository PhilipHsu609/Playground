# Projects

A workspace for miscellaneous projects and experiments.

## Active

| Project | Description | Language | Status |
|---------|-------------|----------|--------|
| [interpreter](./interpreter/) | Monkey language from *Writing An Interpreter In Go* + *Writing A Compiler In Go* | C++23 | Interpreter complete, compiler + bytecode VM next |
| LLVM Kaleidoscope | [LLVM tutorial](https://llvm.org/docs/tutorial/) — IR generation, JIT, optimization passes, native codegen | C++ | Not started |

Compiler reference: [An Incremental Approach to Compiler Construction](https://github.com/namin/inc) (Ghuloum) for x86 codegen fundamentals.

## Roadmap: distributed KV store

A from-scratch distributed, fault-tolerant, networked key-value database, built
in three linked stages (each a standalone, demoable artifact):

1. **Storage engine** — durable single-node KV store: **LSM-tree first**
   (memtable, WAL, SSTables, compaction), then optionally a **B-tree** for
   comparison. Exposes a state-machine interface (`apply(command)` / `get`) so
   Raft can drive it.
2. **Raft** — replicates that state machine across nodes (leader election, log
   replication). Calls the storage engine's `apply` on commit.
3. **HTTP server** — the client-facing front door; turns requests into Raft
   `propose`/`get` calls. Built on **`io_uring`** (modern high-throughput async
   I/O) rather than the epoll/sockets stack already done before.

Build to the seams so each stage snaps onto the next without rewrites.

## Completed

| Project | Description | Language |
|---------|-------------|----------|
| [tinyrenderer](./tinyrenderer/) | Software renderer following [haqr.eu/tinyrenderer](https://haqr.eu/tinyrenderer/) — lessons 1–11 (through toon shading) | C++23 |
| [container](./container/) | Linux container runtime from scratch | C |
| [sELF](./sELF/) | ELF binary format parser | C |

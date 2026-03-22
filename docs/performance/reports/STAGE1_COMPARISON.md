# Stage 1 Comparison

Generated on 2026-03-22 09:52:57 UTC.

This table keeps the stage-1 lanes separate on purpose:
- `VM` rows measure the direct typed-value VM kernel (`vm_dispatch`).
- `Graphion (.gion)` rows measure the `.gion` source workload end-to-end (`gion_stage1`).
- `Rust` rows measure the Rust comparison lane for the same stage-1 VM kernel family.

| Lane | Workload | s | Throughput | Latency | Compiler | ASM |
|---|---|---:|---:|---:|---|---|
| VM (Windows) | vm_dispatch | 0.019702 | 406.163 mips | 2.463 ns/instruction | msvc | off |
| VM (Linux) | vm_dispatch | 0.030772 | 261.264 mips | 3.847 ns/instruction | gcc | on |
| Graphion (.gion) (Windows) | gion_stage1 | 0.096917 | 2.481 mops | 403.821 ns/operation | msvc | off |
| Graphion (.gion) (Linux) | gion_stage1 | 0.128767 | 1.866 mops | 536.528 ns/operation | gcc | on |
| Rust (Windows) | vm_dispatch | 0.030187 | 265.319 mips | 3.773 ns/instruction | rustc | off |
| Rust (Linux) | vm_dispatch | 0.033535 | 239.050 mips | 4.192 ns/instruction | rustc | off |

Notes:

- Missing rows mean the corresponding lane was not collected on this machine or in Docker.
- `Graphion (.gion)` includes frontend/runtime overhead that the direct VM lane intentionally does not include.

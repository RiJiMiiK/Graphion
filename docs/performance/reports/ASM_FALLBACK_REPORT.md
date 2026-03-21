# ASM Fallback Report

Generated: 2026-03-06T22:39:15.887777+00:00

This report reflects the latest local asm-vs-C comparison run. It is a policy and parity checkpoint, not an automatic release baseline.

## Metadata

- Platform: linux
- Compiler kind: auto
- Config: Release
- Iterations: 2000
- Runs: 1

## Parity

- C fallback tests: pass
- ASM tests: pass
- Benchmark checksum parity: pass

## Performance

| Benchmark | C s | ASM s | C ns_per_X | ASM ns_per_X | C thr | ASM thr | Speedup x | Min x | Target x | Status |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| vm_dispatch | 0.000027 | 0.000024 | 0.742 | 0.676 | 1348.169 | 1480.343 | 1.098 | 1.050 | 1.150 | meets-minimum |
| vm_graph_ops | 0.000058 | 0.000068 | 2.921 | 3.409 | 342.392 | 293.308 | 0.857 | 0.980 | 1.000 | below-minimum |

## Policy

- The C path remains the semantic reference implementation.
- Assembly is acceptable only if unit tests pass on both builds and benchmark checksums match.
- `vm_dispatch` must show measured improvement to justify the extra maintenance cost of asm.
- Non-targeted workloads may not materially regress when asm is enabled.

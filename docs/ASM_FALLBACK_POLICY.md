# ASM Fallback Policy

## Goal

Keep the portable C hotpath as the semantic reference and require assembly to justify its maintenance cost with measured data.

## Core Rules

- the C fallback is the reference implementation
- assembly must never replace the need for parity checks against the C path
- assembly is accepted only for proven bottlenecks
- assembly may be disabled at any time if parity or performance evidence stops being convincing

## Required Evidence

Any assembly hotpath change must provide both:

- semantic parity evidence
  - `ctest` passes with `GRAPHION_ENABLE_ASM=OFF`
  - `ctest` passes with `GRAPHION_ENABLE_ASM=ON`
  - benchmark checksums match between C and asm builds
- performance evidence
  - `vm_dispatch` must meet at least `1.05x` speedup versus the C fallback
  - target expectation for `vm_dispatch` is `1.15x`
  - non-targeted `vm_graph_ops` must not regress below `0.98x`

## Reporting

Use the dedicated comparison runner:

```bash
python3 scripts/bench/compare_asm_fallback.py \
  --build-root build-asm-fallback \
  --runs 20 \
  --iterations 500000 \
  -- -G Ninja -DCMAKE_C_COMPILER=gcc
```

On Windows, run the comparison inside the project Docker environment because the current asm backend targets x86_64 SysV:

```powershell
docker compose run --rm graphion-dev bash -lc \
  "python3 scripts/bench/compare_asm_fallback.py --build-root build-asm-fallback --runs 20 --iterations 500000 -- -G Ninja -DCMAKE_C_COMPILER=gcc"
```

The command generates:

- `benchmarks/results/asm_fallback_report_latest.json`
- `docs/ASM_FALLBACK_REPORT.md`

The Markdown report is the latest local comparison snapshot. It should be treated as evidence for review, not as a release baseline unless it is regenerated with the intended benchmark scale.

For the current Linux asm path, `gcc` is the reference compiler for this comparison flow.
Clang may require extra assembler-flag handling because GNU-style `.s` files plus global `-Werror` can reject unused compile flags.

## Review Guidance

- if parity fails, asm is not shippable
- if `vm_dispatch` fails the minimum speedup, asm is not justified
- if non-targeted workloads regress materially, the asm path must be reconsidered or narrowed
- if the evidence is noisy, rerun with more iterations before making a policy decision

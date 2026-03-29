# Graphion Documentation

Welcome to the Graphion documentation site.

This documentation is split into two main tracks:

- **Graphion user documentation**
  for the `.gion` language and its builtins
- **Runtime and engineering documentation**
  for the VM, ISA, performance, debugging, and rebuild constraints

The recommended reading order for the current project state is:

1. the Graphion user guide
2. the rebuild charter
3. the architecture page
4. the ISA and benchmark docs if you are working on the runtime

```{toctree}
:maxdepth: 2
:caption: Graphion

graphion/index
```

```{toctree}
:maxdepth: 2
:caption: Engineering Docs

runtime/core/ARCHITECTURE
runtime/core/ISA
runtime/core/REBUILD_CHARTER
runtime/debugging/ERRORS
performance/guides/BENCHMARKS
performance/guides/PGO
performance/reports/PERFORMANCE_RESULTS
```

# Graphion Documentation

Welcome to the Graphion documentation site.

Current project state:

- the documented `.gion` frontend currently covers the implemented scalar subset
- the repository also contains broader VM work for graph and hypergraph execution
- the active repo lane is currently hygiene and maintenance rather than new language-surface expansion

This documentation is split into two main tracks:

- **Graphion user documentation**
  for the `.gion` language and its builtins
- **Runtime and engineering documentation**
  for the VM, ISA, debugging, and rebuild constraints

The recommended reading order for the current project state is:

1. the Graphion user guide
2. the rebuild charter
3. the architecture page
4. the ISA and errors docs if you are working on the runtime

```{toctree}
:maxdepth: 2
:caption: Graphion

graphion/index
```

```{toctree}
:maxdepth: 2
:caption: Engineering Docs

runtime/index
```

/* SPDX-License-Identifier: MIT */

#include "parser/stdlib.h"

#include <stddef.h>
#include <string.h>

static const graphion_stdlib_program GRAPHION_STDLIB_PROGRAMS[] = {
    {
        "graph.neighbors.materialize",
        "Materialize the outgoing neighbors of node r0 into the output frontier.",
        "neighbors_of r0, 0\n"
        "halt\n",
        1,
        0,
        1,
    },
    {
        "graph.neighbors.expand",
        "Expand the bound input frontier across outgoing CSR neighbors.",
        "neighbors_expand r0, 0\n"
        "halt\n",
        1,
        0,
        1,
    },
    {
        "graph.weights.sum",
        "Sum the outgoing edge weights of node r0 into r1.",
        "neighbor_weight_sum r0, r1\n"
        "halt\n",
        1,
        0,
        0,
    },
    {
        "graph.attrs.sum",
        "Sum the outgoing edge attributes of node r0 into r1.",
        "neighbor_attr_sum r0, r1\n"
        "halt\n",
        1,
        0,
        0,
    },
    {
        "graph.bfs.levels",
        "Run BFS from source node r0 and write the visited-level count to r1.",
        "bfs_levels r0, r1\n"
        "halt\n",
        1,
        0,
        0,
    },
    {
        "hypergraph.incident.count",
        "Count hyperedges incident to node r0 into r1.",
        "incident_count r0, r1\n"
        "halt\n",
        0,
        1,
        0,
    },
    {
        "hypergraph.incident.materialize",
        "Materialize hyperedges incident to node r0 into the output frontier.",
        "incident_of r0, 0\n"
        "halt\n",
        0,
        1,
        1,
    },
    {
        "hypergraph.hyperedge.size",
        "Count nodes contained in hyperedge r0 into r1.",
        "hyperedge_size r0, r1\n"
        "halt\n",
        0,
        1,
        0,
    },
    {
        "hypergraph.hyperedge.materialize_nodes",
        "Materialize the nodes of hyperedge r0 into the output frontier.",
        "hyperedge_nodes_of r0, 0\n"
        "halt\n",
        0,
        1,
        1,
    },
    {
        "hypergraph.incident.sum",
        "Sum incident hyperedge ids for node r0 into r1.",
        "incident_sum r0, r1\n"
        "halt\n",
        0,
        1,
        0,
    },
    {
        "hypergraph.hyperedge.node_sum",
        "Sum node ids contained in hyperedge r0 into r1.",
        "hyperedge_node_sum r0, r1\n"
        "halt\n",
        0,
        1,
        0,
    },
};

size_t graphion_stdlib_program_count(void) {
  return sizeof(GRAPHION_STDLIB_PROGRAMS) / sizeof(GRAPHION_STDLIB_PROGRAMS[0]);
}

const graphion_stdlib_program *graphion_stdlib_program_at(size_t index) {
  if (index >= graphion_stdlib_program_count()) {
    return NULL;
  }
  return &GRAPHION_STDLIB_PROGRAMS[index];
}

const graphion_stdlib_program *graphion_stdlib_find_program(const char *name) {
  size_t i;
  if (name == NULL) {
    return NULL;
  }
  for (i = 0U; i < graphion_stdlib_program_count(); ++i) {
    if (strcmp(name, GRAPHION_STDLIB_PROGRAMS[i].name) == 0) {
      return &GRAPHION_STDLIB_PROGRAMS[i];
    }
  }
  return NULL;
}

int graphion_stdlib_lower_program_to_ir(const char *name,
                                        graphion_ir_insn *out_ir,
                                        size_t out_capacity,
                                        size_t *out_count,
                                        graphion_frontend_diagnostic *diagnostic) {
  const graphion_stdlib_program *program = graphion_stdlib_find_program(name);
  if (program == NULL) {
    if (diagnostic != NULL) {
      diagnostic->code = GFE_DIAG_INVALID_ARGUMENT;
      diagnostic->start.line = 0U;
      diagnostic->start.column = 0U;
      diagnostic->end.line = 0U;
      diagnostic->end.column = 0U;
      diagnostic->message = "unknown stdlib program";
    }
    if (out_count != NULL) {
      *out_count = 0U;
    }
    return GFE_ERR_INVALID_ARG;
  }
  return graphion_parse_source_to_ir_with_diagnostic(
      program->source, out_ir, out_capacity, out_count, diagnostic);
}

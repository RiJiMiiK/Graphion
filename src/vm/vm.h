/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_VM_H
#define GRAPHION_VM_VM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "graph/csr_graph.h"
#include "graph/hypergraph.h"

typedef enum {
  GVM_OP_NOP = 0,
  GVM_OP_HALT = 1,
  GVM_OP_MOV_IMM = 2,
  GVM_OP_ADD = 3,
  GVM_OP_MOV = 4,
  GVM_OP_LOAD_CONST = 5,
  GVM_OP_LOAD_GLOBAL = 6,
  GVM_OP_STORE_GLOBAL = 7,
  GVM_OP_STORE_CONST_GLOBAL = 8,
  GVM_OP_COPY_GLOBAL = 9,
  GVM_OP_PRINT_CONST = 10,
  GVM_OP_PRINT_GLOBAL = 11,
  GVM_OP_PRINT_REG = 12,
  GVM_OP_SUB = 13,
  GVM_OP_MUL = 14,
  GVM_OP_DIV = 15,
  GVM_OP_POW = 24,
  GVM_OP_MOD = 23,
  GVM_OP_PRINT_CONST_PART = 25,
  GVM_OP_PRINT_GLOBAL_PART = 26,
  GVM_OP_PRINT_REG_PART = 27,
  GVM_OP_PRINT_NEWLINE = 28,
  GVM_OP_FLOOR_DIV = 29,
  GVM_OP_ABS = 30,
  GVM_OP_EQ = 31,
  GVM_OP_FRONTIER_CLEAR = 32,
  GVM_OP_FRONTIER_PUSH = 33,
  GVM_OP_FRONTIER_FILTER_LT_IMM = 34,
  GVM_OP_FRONTIER_MAP_ADD_IMM = 35,
  GVM_OP_FRONTIER_REDUCE_SUM = 36,
  GVM_OP_FRONTIER_SWAP = 37,
  GVM_OP_NEIGHBORS_OF = 38,
  GVM_OP_NEIGHBORS_EXPAND = 39,
  GVM_OP_INCIDENT_OF = 40,
  GVM_OP_HYPEREDGE_NODES_OF = 41,
  GVM_OP_NEIGHBOR_WEIGHT_SUM = 42,
  GVM_OP_NEIGHBOR_ATTR_SUM = 43,
  GVM_OP_NE = 44,
  GVM_OP_LT = 45,
  GVM_OP_LE = 46,
  GVM_OP_GT = 47,
  GVM_OP_GE = 48,
  GVM_OP_AND = 49,
  GVM_OP_OR = 50,
  GVM_OP_NOT = 51,
  GVM_OP_NAND = 52,
  GVM_OP_NOR = 53,
  GVM_OP_JUMP = 54,
  GVM_OP_JUMP_IF_TRUE = 55,
  GVM_OP_JUMP_IF_FALSE = 56,
  GVM_OP_BIT_AND = 57,
  GVM_OP_BIT_OR = 58,
  GVM_OP_BIT_XOR = 59,
  GVM_OP_BIT_NOT = 60,
  GVM_OP_BIT_SHL = 61,
  GVM_OP_BIT_SHR = 62,
  GVM_OP_MIN = 63,
  GVM_OP_MAX = 64,
  GVM_OP_CLAMP = 65,
  GVM_OP_SQRT = 66,
  GVM_OP_CBRT = 67,
  GVM_OP_SIN = 68,
  GVM_OP_SINH = 69,
  GVM_OP_ASINH = 70,
  GVM_OP_ACOSH = 71,
  GVM_OP_COSH = 72,
  GVM_OP_TANH = 73,
  GVM_OP_ATANH = 74,
  GVM_OP_COS = 75,
  GVM_OP_TAN = 76,
  GVM_OP_ASIN = 77,
  GVM_OP_ACOS = 78,
  GVM_OP_ATAN = 79,
  GVM_OP_ATAN2 = 80,
  GVM_OP_HYPOT = 81,
  GVM_OP_EXP = 82,
  GVM_OP_LN = 83,
  GVM_OP_LOG = 84,
  GVM_OP_FLOOR = 85,
  GVM_OP_CEIL = 86,
  GVM_OP_ROUND = 87,
  GVM_OP_TRUNC = 88,
  GVM_OP_SIGN = 89,
  GVM_OP_LEN = 90,
  GVM_OP_FACTORIAL = 91,
  GVM_OP_DEGREES = 92,
  GVM_OP_RADIANS = 93,
  GVM_OP_ISNAN = 94,
  GVM_OP_ISINF = 95,
  GVM_OP_ISFINITE = 96,
  GVM_OP_FRACT = 97,
  GVM_OP_BFS_LEVELS = 16,
  GVM_OP_INCIDENT_COUNT = 17,
  GVM_OP_HYPEREDGE_SIZE = 18,
  GVM_OP_INCIDENT_SUM = 19,
  GVM_OP_HYPEREDGE_NODE_SUM = 20,
  GVM_OP_BFS_LEVEL_COUNT = 21,
  GVM_OP_BFS_ORDER = 22
} graphion_opcode;

typedef enum {
  GVM_OK = 0,
  GVM_ERR_INVALID_ARG = -1,
  GVM_ERR_INVALID_MOV_IMM_REG = -2,
  GVM_ERR_INVALID_REG = -3,
  GVM_ERR_UNKNOWN_OPCODE = -4,
  GVM_ERR_CSR_UNBOUND = -5,
  GVM_ERR_INVALID_BFS_SOURCE = -6,
  GVM_ERR_BFS_RUNTIME = -7,
  GVM_ERR_HYPERGRAPH_UNBOUND = -8,
  GVM_ERR_INVALID_NODE_ID = -9,
  GVM_ERR_INVALID_HYPEREDGE_ID = -10,
  GVM_ERR_FRONTIER_UNBOUND = -11,
  GVM_ERR_FRONTIER_OVERFLOW = -12,
  GVM_ERR_INVALID_FRONTIER_VALUE = -13,
  GVM_ERR_CSR_WEIGHTS_UNBOUND = -14,
  GVM_ERR_CSR_EDGE_ATTRS_UNBOUND = -15,
  GVM_ERR_TYPE_MISMATCH = -16,
  GVM_ERR_CONST_UNBOUND = -17,
  GVM_ERR_GLOBALS_UNBOUND = -18,
  GVM_ERR_INVALID_CONST_INDEX = -19,
  GVM_ERR_INVALID_GLOBAL_INDEX = -20,
  GVM_ERR_OUTPUT_UNBOUND = -21,
  GVM_ERR_DIVIDE_BY_ZERO = -22,
  GVM_ERR_DOMAIN = -23,
  GVM_ERR_FACTORIAL_DOMAIN = -24,
  GVM_ERR_LN_DOMAIN = -25,
  GVM_ERR_LOG_DOMAIN = -26,
  GVM_ERR_ASIN_DOMAIN = -27,
  GVM_ERR_ACOS_DOMAIN = -28,
  GVM_ERR_ACOSH_DOMAIN = -29,
  GVM_ERR_ATANH_DOMAIN = -30
} graphion_vm_result;

typedef enum {
  GVM_VALUE_NONE = 0,
  GVM_VALUE_INT = 1,
  GVM_VALUE_FLOAT = 2,
  GVM_VALUE_BOOL = 3,
  GVM_VALUE_STRING = 4,
  GVM_VALUE_GRAPH_REF = 5,
  GVM_VALUE_HYPERGRAPH_REF = 6,
  GVM_VALUE_INT_SEQUENCE_REF = 7,
  GVM_VALUE_BITS = 8
} graphion_vm_value_kind;

typedef struct {
  uint8_t kind;
  uint8_t reserved[7];
  union {
    int64_t int_value;
    double float_value;
    int bool_value;
    const char *string_value;
    const void *ref_value;
  } as;
} graphion_vm_value;

typedef struct {
  uint8_t op;
  uint8_t a;
  uint8_t b;
  int32_t imm;
} graphion_insn;

typedef int (*graphion_output_write_fn)(void *ctx, const char *bytes, size_t len);

typedef struct {
  graphion_output_write_fn write;
  void *ctx;
} graphion_output_sink;

typedef struct {
  graphion_vm_value regs[16];
  char *owned_reg_strings[16];
  const graphion_insn *program;
  size_t program_len;
  size_t pc;
  bool halted;
  bool deterministic_mode;
  bool arith_only_fastpath;
  bool arith_only_halt_terminated;
  bool weighted_sum_fastpath;
  bool frontier_filter_map_reduce_fastpath;
  bool frontier_fastpath;
  bool graph_ops_fastpath;
  bool value_move_fastpath;
  bool global_materialize_fastpath;
  bool global_print_fastpath;
  bool value_move_indices_valid;
  bool value_move_int_add_safe;
  bool global_print_indices_valid;
  const graphion_vm_value *const_pool;
  size_t const_count;
  graphion_vm_value *globals;
  char **global_string_owners;
  size_t global_count;
  graphion_output_sink output;
  size_t global_print_const_lens[512];
  size_t global_print_global_lens[256];
  const graphion_csr_graph *csr_graph;
  int32_t *bfs_levels;
  uint32_t *bfs_queue;
  size_t bfs_capacity;
  const graphion_hypergraph *hypergraph;
  uint32_t *frontier_input;
  size_t frontier_input_len;
  uint32_t *frontier_output;
  size_t frontier_output_len;
  size_t frontier_capacity;
} graphion_vm;

void graphion_vm_init(graphion_vm *vm);
void graphion_vm_dispose(graphion_vm *vm);
void graphion_vm_reset_execution(graphion_vm *vm);
void graphion_vm_set_deterministic(graphion_vm *vm, bool enabled);
int graphion_vm_load(graphion_vm *vm, const graphion_insn *program, size_t program_len);
void graphion_vm_bind_csr(graphion_vm *vm,
                          const graphion_csr_graph *graph,
                          int32_t *bfs_levels,
                          uint32_t *bfs_queue,
                          size_t bfs_capacity);
void graphion_output_sink_from_file(graphion_output_sink *sink, FILE *output);
void graphion_output_sink_from_counter(graphion_output_sink *sink, uint64_t *byte_count);
void graphion_vm_bind_constants(graphion_vm *vm, const graphion_vm_value *const_pool, size_t const_count);
void graphion_vm_bind_globals(graphion_vm *vm, graphion_vm_value *globals, size_t global_count);
void graphion_vm_bind_global_string_owners(graphion_vm *vm, char **owners, size_t owner_count);
void graphion_vm_bind_output_sink(graphion_vm *vm, const graphion_output_sink *output);
void graphion_vm_bind_output(graphion_vm *vm, FILE *output);
void graphion_vm_bind_hypergraph(graphion_vm *vm, const graphion_hypergraph *graph);
void graphion_vm_bind_frontier(graphion_vm *vm,
                               uint32_t *input,
                               size_t input_len,
                               uint32_t *output,
                               size_t capacity);
int graphion_vm_run(graphion_vm *vm);
size_t graphion_vm_write_snapshot(const graphion_vm *vm, char *buffer, size_t buffer_size);

#endif

/* SPDX-License-Identifier: MIT */

#include "parser/frontend.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { GFE_LINE_MAX = 256 };

typedef struct {
  const char *mnemonic;
  uint8_t opcode;
  int kind;
} op_spec;

enum {
  OP_KIND_ZERO = 0,
  OP_KIND_REG_IMM = 1,
  OP_KIND_REG_REG = 2
};

static int streq_icase(const char *a, const char *b) {
  const char *pa = a;
  const char *pb = b;
  while (*pa != '\0' && *pb != '\0') {
    const unsigned char ca = (unsigned char)(*pa);
    const unsigned char cb = (unsigned char)(*pb);
    if (tolower(ca) != tolower(cb)) {
      return 0;
    }
    ++pa;
    ++pb;
  }
  return *pa == '\0' && *pb == '\0';
}

static const op_spec *find_op_spec(const char *mnemonic) {
  static const op_spec specs[] = {
      {"nop", GIR_OP_NOP, OP_KIND_ZERO},
      {"halt", GIR_OP_HALT, OP_KIND_ZERO},
      {"mov", GIR_OP_MOV_IMM, OP_KIND_REG_IMM},
      {"mov_imm", GIR_OP_MOV_IMM, OP_KIND_REG_IMM},
      {"add", GIR_OP_ADD, OP_KIND_REG_REG},
      {"bfs_levels", GIR_OP_BFS_LEVELS, OP_KIND_REG_REG},
      {"incident_count", GIR_OP_INCIDENT_COUNT, OP_KIND_REG_REG},
      {"hyperedge_size", GIR_OP_HYPEREDGE_SIZE, OP_KIND_REG_REG},
      {"incident_sum", GIR_OP_INCIDENT_SUM, OP_KIND_REG_REG},
      {"hyperedge_node_sum", GIR_OP_HYPEREDGE_NODE_SUM, OP_KIND_REG_REG},
  };
  size_t i;
  for (i = 0U; i < (sizeof(specs) / sizeof(specs[0])); ++i) {
    if (streq_icase(specs[i].mnemonic, mnemonic)) {
      return &specs[i];
    }
  }
  return NULL;
}

static void trim_in_place(char *s) {
  size_t start = 0U;
  size_t end;
  size_t i;
  while (s[start] != '\0' && isspace((unsigned char)s[start]) != 0) {
    ++start;
  }
  if (start > 0U) {
    i = 0U;
    while (s[start + i] != '\0') {
      s[i] = s[start + i];
      ++i;
    }
    s[i] = '\0';
  }
  end = strlen(s);
  while (end > 0U && isspace((unsigned char)s[end - 1U]) != 0) {
    --end;
  }
  s[end] = '\0';
}

static void strip_comments(char *s) {
  size_t i = 0U;
  while (s[i] != '\0') {
    if (s[i] == '#') {
      s[i] = '\0';
      break;
    }
    if (s[i] == '/' && s[i + 1U] == '/') {
      s[i] = '\0';
      break;
    }
    ++i;
  }
}

static int parse_register(const char *tok, uint8_t *out_reg) {
  long v;
  char *end = NULL;
  if (tok == NULL || out_reg == NULL) {
    return 0;
  }
  if (!(tok[0] == 'r' || tok[0] == 'R')) {
    return 0;
  }
  if (tok[1] == '\0') {
    return 0;
  }
  v = strtol(tok + 1, &end, 10);
  if (end == NULL || *end != '\0' || v < 0L || v > 255L) {
    return 0;
  }
  *out_reg = (uint8_t)v;
  return 1;
}

static int parse_i32(const char *tok, int32_t *out_imm) {
  long v;
  char *end = NULL;
  if (tok == NULL || out_imm == NULL) {
    return 0;
  }
  v = strtol(tok, &end, 10);
  if (end == NULL || *end != '\0') {
    return 0;
  }
  if (v < (long)INT32_MIN || v > (long)INT32_MAX) {
    return 0;
  }
  *out_imm = (int32_t)v;
  return 1;
}

static int is_sep(char c) { return c == ' ' || c == '\t' || c == ','; }

static int split_tokens(char *line, char *tokens[3], size_t *out_count) {
  size_t count = 0U;
  char *p;
  if (line == NULL || tokens == NULL || out_count == NULL) {
    return 0;
  }

  p = line;
  while (*p != '\0') {
    while (*p != '\0' && is_sep(*p)) {
      ++p;
    }
    if (*p == '\0') {
      break;
    }
    if (count >= 3U) {
      return 0;
    }
    tokens[count++] = p;
    while (*p != '\0' && !is_sep(*p)) {
      ++p;
    }
    if (*p == '\0') {
      break;
    }
    *p = '\0';
    ++p;
  }

  *out_count = count;
  return 1;
}

int graphion_parse_source_to_ir(const char *source,
                                graphion_ir_insn *out_ir,
                                size_t out_capacity,
                                size_t *out_count) {
  const char *p;
  size_t produced = 0U;
  if (source == NULL || out_ir == NULL || out_count == NULL) {
    return GFE_ERR_INVALID_ARG;
  }

  p = source;
  while (*p != '\0') {
    char line[GFE_LINE_MAX];
    size_t n = 0U;
    char *tokens[3] = {NULL, NULL, NULL};
    size_t token_count = 0U;
    graphion_ir_insn insn = {0U, 0U, 0U, 0};
    const op_spec *spec;
    while (*p != '\0' && *p != '\n' && n < (GFE_LINE_MAX - 1U)) {
      line[n++] = *p++;
    }
    if (*p == '\n') {
      ++p;
    }
    line[n] = '\0';
    strip_comments(line);
    trim_in_place(line);
    if (line[0] == '\0') {
      continue;
    }
    if (!split_tokens(line, tokens, &token_count) || token_count == 0U) {
      return GFE_ERR_PARSE;
    }
    spec = find_op_spec(tokens[0]);
    if (spec == NULL) {
      return GFE_ERR_PARSE;
    }
    insn.op = spec->opcode;

    if (spec->kind == OP_KIND_ZERO) {
      if (token_count != 1U) {
        return GFE_ERR_PARSE;
      }
    } else if (spec->kind == OP_KIND_REG_IMM) {
      if (token_count != 3U || !parse_register(tokens[1], &insn.a) || !parse_i32(tokens[2], &insn.imm)) {
        return GFE_ERR_PARSE;
      }
    } else if (spec->kind == OP_KIND_REG_REG) {
      if (token_count != 3U || !parse_register(tokens[1], &insn.a) || !parse_register(tokens[2], &insn.b)) {
        return GFE_ERR_PARSE;
      }
    } else {
      return GFE_ERR_PARSE;
    }

    if (produced >= out_capacity) {
      return GFE_ERR_CAPACITY;
    }
    out_ir[produced++] = insn;
  }

  *out_count = produced;
  return GFE_OK;
}

/* SPDX-License-Identifier: MIT */

#include "parser/frontend.h"

#include "parser/lexer.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

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

enum { GFE_TOKEN_CAPACITY = 512 };

typedef struct {
  const graphion_token *tokens;
  size_t count;
  size_t index;
} parser_cursor;

static int streq_token_icase(const char *source, const graphion_token *tok, const char *literal) {
  size_t i = 0U;
  if (tok->kind != GTOK_IDENTIFIER) {
    return 0;
  }
  while (i < tok->length && literal[i] != '\0') {
    const unsigned char a = (unsigned char)source[tok->offset + i];
    const unsigned char b = (unsigned char)literal[i];
    if (tolower(a) != tolower(b)) {
      return 0;
    }
    ++i;
  }
  return i == tok->length && literal[i] == '\0';
}

static const op_spec *find_op_spec(const char *source, const graphion_token *tok) {
  static const op_spec specs[] = {
      {"nop", GIR_OP_NOP, OP_KIND_ZERO},
      {"halt", GIR_OP_HALT, OP_KIND_ZERO},
      {"mov", GIR_OP_MOV_IMM, OP_KIND_REG_IMM},
      {"mov_imm", GIR_OP_MOV_IMM, OP_KIND_REG_IMM},
      {"add", GIR_OP_ADD, OP_KIND_REG_REG},
      {"frontier_clear", GIR_OP_FRONTIER_CLEAR, OP_KIND_REG_IMM},
      {"frontier_push", GIR_OP_FRONTIER_PUSH, OP_KIND_REG_REG},
      {"frontier_filter_lt_imm", GIR_OP_FRONTIER_FILTER_LT_IMM, OP_KIND_REG_IMM},
      {"frontier_map_add_imm", GIR_OP_FRONTIER_MAP_ADD_IMM, OP_KIND_REG_IMM},
      {"frontier_reduce_sum", GIR_OP_FRONTIER_REDUCE_SUM, OP_KIND_REG_IMM},
      {"frontier_swap", GIR_OP_FRONTIER_SWAP, OP_KIND_REG_IMM},
      {"neighbors_of", GIR_OP_NEIGHBORS_OF, OP_KIND_REG_IMM},
      {"neighbors_expand", GIR_OP_NEIGHBORS_EXPAND, OP_KIND_REG_IMM},
      {"incident_of", GIR_OP_INCIDENT_OF, OP_KIND_REG_IMM},
      {"hyperedge_nodes_of", GIR_OP_HYPEREDGE_NODES_OF, OP_KIND_REG_IMM},
      {"neighbor_weight_sum", GIR_OP_NEIGHBOR_WEIGHT_SUM, OP_KIND_REG_REG},
      {"neighbor_attr_sum", GIR_OP_NEIGHBOR_ATTR_SUM, OP_KIND_REG_REG},
      {"bfs_levels", GIR_OP_BFS_LEVELS, OP_KIND_REG_REG},
      {"incident_count", GIR_OP_INCIDENT_COUNT, OP_KIND_REG_REG},
      {"hyperedge_size", GIR_OP_HYPEREDGE_SIZE, OP_KIND_REG_REG},
      {"incident_sum", GIR_OP_INCIDENT_SUM, OP_KIND_REG_REG},
      {"hyperedge_node_sum", GIR_OP_HYPEREDGE_NODE_SUM, OP_KIND_REG_REG},
  };
  size_t i;
  for (i = 0U; i < sizeof(specs) / sizeof(specs[0]); ++i) {
    if (streq_token_icase(source, tok, specs[i].mnemonic)) {
      return &specs[i];
    }
  }
  return NULL;
}

static const graphion_token *peek_token(const parser_cursor *cursor) {
  if (cursor->index >= cursor->count) {
    return NULL;
  }
  return &cursor->tokens[cursor->index];
}

static void advance_token(parser_cursor *cursor) {
  if (cursor->index < cursor->count) {
    ++cursor->index;
  }
}

static void set_error_pos(graphion_frontend_position *error_pos, const graphion_token *tok) {
  if (error_pos == NULL || tok == NULL) {
    return;
  }
  error_pos->line = tok->line;
  error_pos->column = tok->column;
}

static int expect_token_kind(parser_cursor *cursor,
                             uint8_t kind,
                             graphion_frontend_position *error_pos,
                             const graphion_token **out_tok) {
  const graphion_token *tok = peek_token(cursor);
  if (tok == NULL || tok->kind != kind) {
    set_error_pos(error_pos, tok);
    return GFE_ERR_PARSE;
  }
  if (out_tok != NULL) {
    *out_tok = tok;
  }
  advance_token(cursor);
  return GFE_OK;
}

static void skip_newlines(parser_cursor *cursor) {
  const graphion_token *tok = peek_token(cursor);
  while (tok != NULL && tok->kind == GTOK_NEWLINE) {
    advance_token(cursor);
    tok = peek_token(cursor);
  }
}

static int parse_instruction(const char *source,
                             parser_cursor *cursor,
                             graphion_ir_insn *insn,
                             graphion_frontend_position *error_pos) {
  const graphion_token *mnemonic_tok = NULL;
  const graphion_token *a_tok = NULL;
  const graphion_token *b_tok = NULL;
  const op_spec *spec;
  int rc;

  rc = expect_token_kind(cursor, GTOK_IDENTIFIER, error_pos, &mnemonic_tok);
  if (rc != GFE_OK) {
    return rc;
  }
  spec = find_op_spec(source, mnemonic_tok);
  if (spec == NULL) {
    set_error_pos(error_pos, mnemonic_tok);
    return GFE_ERR_PARSE;
  }

  insn->op = spec->opcode;
  insn->a = 0U;
  insn->b = 0U;
  insn->imm = 0;

  if (spec->kind == OP_KIND_ZERO) {
    return GFE_OK;
  }

  rc = expect_token_kind(cursor, GTOK_REGISTER, error_pos, &a_tok);
  if (rc != GFE_OK) {
    return rc;
  }
  insn->a = a_tok->reg_value;

  rc = expect_token_kind(cursor, GTOK_COMMA, error_pos, NULL);
  if (rc != GFE_OK) {
    return rc;
  }

  if (spec->kind == OP_KIND_REG_IMM) {
    rc = expect_token_kind(cursor, GTOK_INTEGER, error_pos, &b_tok);
    if (rc != GFE_OK) {
      return rc;
    }
    insn->imm = (int32_t)b_tok->int_value;
    return GFE_OK;
  }

  rc = expect_token_kind(cursor, GTOK_REGISTER, error_pos, &b_tok);
  if (rc != GFE_OK) {
    return rc;
  }
  insn->b = b_tok->reg_value;
  return GFE_OK;
}

int graphion_parse_source_to_ir_with_position(const char *source,
                                              graphion_ir_insn *out_ir,
                                              size_t out_capacity,
                                              size_t *out_count,
                                              graphion_frontend_position *error_pos) {
  graphion_token tokens[GFE_TOKEN_CAPACITY];
  size_t token_count = 0U;
  parser_cursor cursor;
  size_t produced = 0U;
  int rc;

  if (source == NULL || out_ir == NULL || out_count == NULL) {
    return GFE_ERR_INVALID_ARG;
  }
  if (error_pos != NULL) {
    error_pos->line = 0U;
    error_pos->column = 0U;
  }

  rc = graphion_lex_source(source, tokens, GFE_TOKEN_CAPACITY, &token_count);
  if (rc == GLEX_ERR_CAPACITY) {
    return GFE_ERR_CAPACITY;
  }
  if (rc != GLEX_OK) {
    if (error_pos != NULL && token_count > 0U) {
      error_pos->line = tokens[token_count - 1U].line;
      error_pos->column = tokens[token_count - 1U].column;
    }
    return GFE_ERR_PARSE;
  }

  cursor.tokens = tokens;
  cursor.count = token_count;
  cursor.index = 0U;

  skip_newlines(&cursor);
  while (peek_token(&cursor) != NULL && peek_token(&cursor)->kind != GTOK_EOF) {
    const graphion_token *tok = NULL;
    graphion_ir_insn insn;

    if (produced >= out_capacity) {
      return GFE_ERR_CAPACITY;
    }

    rc = parse_instruction(source, &cursor, &insn, error_pos);
    if (rc != GFE_OK) {
      return rc;
    }
    out_ir[produced++] = insn;

    tok = peek_token(&cursor);
    if (tok != NULL && tok->kind != GTOK_NEWLINE && tok->kind != GTOK_EOF) {
      set_error_pos(error_pos, tok);
      return GFE_ERR_PARSE;
    }
    skip_newlines(&cursor);
  }

  *out_count = produced;
  return GFE_OK;
}

int graphion_parse_source_to_ir(const char *source,
                                graphion_ir_insn *out_ir,
                                size_t out_capacity,
                                size_t *out_count) {
  return graphion_parse_source_to_ir_with_position(source, out_ir, out_capacity, out_count, NULL);
}

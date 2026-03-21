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

static void clear_diagnostic(graphion_frontend_diagnostic *diagnostic) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->code = GFE_DIAG_NONE;
  diagnostic->start.line = 0U;
  diagnostic->start.column = 0U;
  diagnostic->end.line = 0U;
  diagnostic->end.column = 0U;
  diagnostic->message = NULL;
}

static void fill_diagnostic(graphion_frontend_diagnostic *diagnostic,
                            int code,
                            const graphion_token *tok,
                            const char *message) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->code = code;
  diagnostic->message = message;
  if (tok == NULL) {
    diagnostic->start.line = 0U;
    diagnostic->start.column = 0U;
    diagnostic->end.line = 0U;
    diagnostic->end.column = 0U;
    return;
  }
  diagnostic->start.line = tok->line;
  diagnostic->start.column = tok->column;
  diagnostic->end.line = tok->line;
  diagnostic->end.column = tok->column + (tok->length > 0U ? (tok->length - 1U) : 0U);
}

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
                             graphion_frontend_diagnostic *diagnostic,
                             int diagnostic_code,
                             const char *diagnostic_message,
                             const graphion_token **out_tok) {
  const graphion_token *tok = peek_token(cursor);
  if (tok == NULL || tok->kind != kind) {
    set_error_pos(error_pos, tok);
    fill_diagnostic(diagnostic, diagnostic_code, tok, diagnostic_message);
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
                             graphion_ast_stmt *stmt,
                             graphion_frontend_position *error_pos,
                             graphion_frontend_diagnostic *diagnostic) {
  const graphion_token *mnemonic_tok = NULL;
  const graphion_token *a_tok = NULL;
  const graphion_token *b_tok = NULL;
  const op_spec *spec;
  int rc;

  rc = expect_token_kind(cursor,
                         GTOK_IDENTIFIER,
                         error_pos,
                         diagnostic,
                         GFE_DIAG_EXPECTED_MNEMONIC,
                         "expected instruction mnemonic",
                         &mnemonic_tok);
  if (rc != GFE_OK) {
    return rc;
  }
  spec = find_op_spec(source, mnemonic_tok);
  if (spec == NULL) {
    set_error_pos(error_pos, mnemonic_tok);
    fill_diagnostic(diagnostic, GFE_DIAG_UNKNOWN_MNEMONIC, mnemonic_tok, "unknown instruction mnemonic");
    return GFE_ERR_PARSE;
  }

  stmt->op = spec->opcode;
  stmt->lhs.kind = GAST_OPERAND_NONE;
  stmt->lhs.reg = 0U;
  stmt->lhs.imm = 0;
  stmt->rhs.kind = GAST_OPERAND_NONE;
  stmt->rhs.reg = 0U;
  stmt->rhs.imm = 0;
  stmt->start.line = mnemonic_tok->line;
  stmt->start.column = mnemonic_tok->column;
  stmt->end.line = mnemonic_tok->line;
  stmt->end.column = mnemonic_tok->column + (mnemonic_tok->length > 0U ? (mnemonic_tok->length - 1U) : 0U);

  if (spec->kind == OP_KIND_ZERO) {
    return GFE_OK;
  }

  rc = expect_token_kind(cursor,
                         GTOK_REGISTER,
                         error_pos,
                         diagnostic,
                         GFE_DIAG_EXPECTED_REGISTER,
                         "expected register operand",
                         &a_tok);
  if (rc != GFE_OK) {
    return rc;
  }
  stmt->lhs.kind = GAST_OPERAND_REGISTER;
  stmt->lhs.reg = a_tok->reg_value;
  stmt->end.line = a_tok->line;
  stmt->end.column = a_tok->column + (a_tok->length > 0U ? (a_tok->length - 1U) : 0U);

  rc = expect_token_kind(cursor,
                         GTOK_COMMA,
                         error_pos,
                         diagnostic,
                         GFE_DIAG_EXPECTED_COMMA,
                         "expected ',' separator",
                         NULL);
  if (rc != GFE_OK) {
    return rc;
  }

  if (spec->kind == OP_KIND_REG_IMM) {
    rc = expect_token_kind(cursor,
                           GTOK_INTEGER,
                           error_pos,
                           diagnostic,
                           GFE_DIAG_EXPECTED_IMMEDIATE,
                           "expected immediate operand",
                           &b_tok);
    if (rc != GFE_OK) {
      return rc;
    }
    stmt->rhs.kind = GAST_OPERAND_IMMEDIATE;
    stmt->rhs.imm = (int32_t)b_tok->int_value;
    stmt->end.line = b_tok->line;
    stmt->end.column = b_tok->column + (b_tok->length > 0U ? (b_tok->length - 1U) : 0U);
    return GFE_OK;
  }

  rc = expect_token_kind(cursor,
                         GTOK_REGISTER,
                         error_pos,
                         diagnostic,
                         GFE_DIAG_EXPECTED_REGISTER,
                         "expected register operand",
                         &b_tok);
  if (rc != GFE_OK) {
    return rc;
  }
  stmt->rhs.kind = GAST_OPERAND_REGISTER;
  stmt->rhs.reg = b_tok->reg_value;
  stmt->end.line = b_tok->line;
  stmt->end.column = b_tok->column + (b_tok->length > 0U ? (b_tok->length - 1U) : 0U);
  return GFE_OK;
}

int graphion_parse_source_to_ast_with_diagnostic(const char *source,
                                                 graphion_ast_stmt *out_ast,
                                                size_t out_capacity,
                                                size_t *out_count,
                                                graphion_frontend_diagnostic *diagnostic) {
  graphion_token tokens[GFE_TOKEN_CAPACITY];
  size_t token_count = 0U;
  parser_cursor cursor;
  size_t produced = 0U;
  size_t lex_error_line = 0U;
  size_t lex_error_column = 0U;
  int rc;

  if (source == NULL || out_ast == NULL || out_count == NULL) {
    clear_diagnostic(diagnostic);
    if (diagnostic != NULL) {
      diagnostic->code = GFE_DIAG_INVALID_ARGUMENT;
      diagnostic->message = "invalid parser arguments";
    }
    return GFE_ERR_INVALID_ARG;
  }
  clear_diagnostic(diagnostic);

  rc = graphion_lex_source_with_position(source,
                                         tokens,
                                         GFE_TOKEN_CAPACITY,
                                         &token_count,
                                         &lex_error_line,
                                         &lex_error_column);
  if (rc == GLEX_ERR_CAPACITY) {
    if (diagnostic != NULL) {
      diagnostic->code = GFE_DIAG_SOURCE_TOO_LARGE;
      diagnostic->start.line = 0U;
      diagnostic->start.column = 0U;
      diagnostic->end.line = 0U;
      diagnostic->end.column = 0U;
      diagnostic->message = "source token stream exceeded parser capacity";
    }
    return GFE_ERR_CAPACITY;
  }
  if (rc != GLEX_OK) {
    if (diagnostic != NULL) {
      diagnostic->code = GFE_DIAG_INVALID_TOKEN;
      diagnostic->start.line = lex_error_line;
      diagnostic->start.column = lex_error_column;
      diagnostic->end.line = lex_error_line;
      diagnostic->end.column = lex_error_column;
      diagnostic->message = "invalid token in source";
    }
    return GFE_ERR_PARSE;
  }

  cursor.tokens = tokens;
  cursor.count = token_count;
  cursor.index = 0U;

  skip_newlines(&cursor);
  while (peek_token(&cursor) != NULL && peek_token(&cursor)->kind != GTOK_EOF) {
    const graphion_token *tok = NULL;
    graphion_ast_stmt stmt;

    if (produced >= out_capacity) {
      return GFE_ERR_CAPACITY;
    }

    rc = parse_instruction(source, &cursor, &stmt, diagnostic != NULL ? &diagnostic->start : NULL, diagnostic);
    if (rc != GFE_OK) {
      if (diagnostic != NULL && diagnostic->end.line == 0U) {
        diagnostic->end = diagnostic->start;
      }
      return rc;
    }
    out_ast[produced++] = stmt;

    tok = peek_token(&cursor);
    if (tok != NULL && tok->kind != GTOK_NEWLINE && tok->kind != GTOK_EOF) {
      fill_diagnostic(diagnostic, GFE_DIAG_EXPECTED_LINE_END, tok, "expected end of line after instruction");
      return GFE_ERR_PARSE;
    }
    skip_newlines(&cursor);
  }

  *out_count = produced;
  return GFE_OK;
}

int graphion_parse_source_to_ir_with_diagnostic(const char *source,
                                                graphion_ir_insn *out_ir,
                                                size_t out_capacity,
                                                size_t *out_count,
                                                graphion_frontend_diagnostic *diagnostic) {
  graphion_ast_stmt ast_program[GFE_TOKEN_CAPACITY];
  size_t ast_count = 0U;
  int rc;

  rc = graphion_parse_source_to_ast_with_diagnostic(source, ast_program, GFE_TOKEN_CAPACITY, &ast_count, diagnostic);
  if (rc != GFE_OK) {
    return rc;
  }

  rc = graphion_ast_lower_to_ir(ast_program, ast_count, out_ir, out_capacity, out_count);
  if (rc == GAST_ERR_CAPACITY) {
    return GFE_ERR_CAPACITY;
  }
  if (rc != GAST_OK) {
    if (diagnostic != NULL) {
      diagnostic->code = GFE_DIAG_INVALID_ARGUMENT;
      diagnostic->message = "internal AST lowering failure";
      diagnostic->start.line = 0U;
      diagnostic->start.column = 0U;
      diagnostic->end.line = 0U;
      diagnostic->end.column = 0U;
    }
    return GFE_ERR_PARSE;
  }
  return GFE_OK;
}

int graphion_parse_source_to_ir_with_position(const char *source,
                                              graphion_ir_insn *out_ir,
                                              size_t out_capacity,
                                              size_t *out_count,
                                              graphion_frontend_position *error_pos) {
  graphion_frontend_diagnostic diagnostic;
  int rc = graphion_parse_source_to_ir_with_diagnostic(source, out_ir, out_capacity, out_count, &diagnostic);
  if (error_pos != NULL) {
    error_pos->line = diagnostic.start.line;
    error_pos->column = diagnostic.start.column;
  }
  return rc;
}

int graphion_parse_source_to_ir(const char *source,
                                graphion_ir_insn *out_ir,
                                size_t out_capacity,
                                size_t *out_count) {
  return graphion_parse_source_to_ir_with_diagnostic(source, out_ir, out_capacity, out_count, NULL);
}

int graphion_parse_source_to_ast(const char *source,
                                 graphion_ast_stmt *out_ast,
                                 size_t out_capacity,
                                 size_t *out_count) {
  return graphion_parse_source_to_ast_with_diagnostic(source, out_ast, out_capacity, out_count, NULL);
}

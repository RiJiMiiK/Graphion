/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  OPERAND_LITERAL = 1,
  OPERAND_GLOBAL = 2
} parsed_operand_kind;

typedef struct {
  parsed_operand_kind kind;
  size_t const_index;
  size_t global_index;
} parsed_operand;

typedef enum {
  EXPR_RESULT_LITERAL = 1,
  EXPR_RESULT_GLOBAL = 2,
  EXPR_RESULT_REG = 3
} parsed_expr_kind;

typedef struct {
  parsed_expr_kind kind;
  size_t const_index;
  size_t global_index;
  uint8_t reg_index;
} parsed_expr_result;

typedef struct {
  char text[512];
  unsigned int line;
  unsigned int indent;
} runtime_source_line;

static void clear_diagnostic(graphion_runtime_diagnostic *diagnostic) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->line = 0U;
  diagnostic->column = 0U;
  diagnostic->message = NULL;
}

static int fail(graphion_runtime_diagnostic *diagnostic,
                unsigned int line,
                unsigned int column,
                const char *message,
                int code) {
  if (diagnostic != NULL) {
    diagnostic->line = line;
    diagnostic->column = column;
    diagnostic->message = message;
  }
  return code;
}

static void vm_value_set_none(graphion_vm_value *value) {
  if (value == NULL) {
    return;
  }
  memset(value, 0, sizeof(*value));
  value->kind = GVM_VALUE_NONE;
}

static void runtime_free_string(char **text) {
  if (text == NULL || *text == NULL) {
    return;
  }
  free(*text);
  *text = NULL;
}

static void skip_spaces(const char **cursor) {
  while (cursor != NULL && *cursor != NULL && (**cursor == ' ' || **cursor == '\t' || **cursor == '\r')) {
    (*cursor)++;
  }
}

static int is_ident_start_char(char ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_';
}

static int is_ident_char(char ch) {
  return is_ident_start_char(ch) || (ch >= '0' && ch <= '9');
}

static int is_reserved_name(const char *name) {
  return strcmp(name, "print") == 0 || strcmp(name, "true") == 0 || strcmp(name, "false") == 0 ||
         strcmp(name, "abs") == 0 || strcmp(name, "if") == 0 || strcmp(name, "elif") == 0 ||
         strcmp(name, "else") == 0;
}

static void copy_name(char dst[GRAPHION_RUNTIME_NAME_MAX], const char *src) {
  size_t len = strlen(src);
  if (len >= GRAPHION_RUNTIME_NAME_MAX) {
    len = GRAPHION_RUNTIME_NAME_MAX - 1U;
  }
  memcpy(dst, src, len);
  dst[len] = '\0';
}

static int scope_find_global_index(const graphion_runtime_scope *scope, const char *name) {
  size_t i;
  if (scope == NULL || name == NULL) {
    return -1;
  }
  for (i = 0U; i < scope->global_count; ++i) {
    if (strcmp(scope->global_names[i], name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int program_find_global_index(const graphion_runtime_program *program, const char *name) {
  size_t i;
  if (program == NULL || name == NULL) {
    return -1;
  }
  for (i = 0U; i < program->global_count; ++i) {
    if (strcmp(program->global_names[i], name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int scope_find_index(const graphion_runtime_scope *scope, const char *name) {
  return scope_find_global_index(scope, name);
}

static int program_find_or_add_global(graphion_runtime_program *program,
                                      const char *name,
                                      unsigned int line,
                                      graphion_runtime_diagnostic *diagnostic,
                                      size_t *index_out) {
  int existing;
  if (program == NULL || name == NULL || index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  existing = program_find_global_index(program, name);
  if (existing >= 0) {
    *index_out = (size_t)existing;
    return GINT_OK;
  }
  if (program->global_count >= GRAPHION_RUNTIME_BINDING_MAX) {
    return fail(diagnostic, line, 1U, "too many globals", GINT_ERR_CAPACITY);
  }
  copy_name(program->global_names[program->global_count], name);
  *index_out = program->global_count;
  program->global_count += 1U;
  return GINT_OK;
}

static int program_add_const(graphion_runtime_program *program,
                             const graphion_vm_value *value,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic,
                             size_t *index_out) {
  if (program == NULL || value == NULL || index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (program->const_count >= GRAPHION_RUNTIME_CONST_MAX) {
    return fail(diagnostic, line, 1U, "constant pool capacity exceeded", GINT_ERR_CAPACITY);
  }
  program->const_pool[program->const_count] = *value;
  *index_out = program->const_count;
  program->const_count += 1U;
  return GINT_OK;
}

static int program_emit(graphion_runtime_program *program,
                        graphion_opcode op,
                        uint8_t a,
                        uint8_t b,
                        int32_t imm,
                        unsigned int line,
                        graphion_runtime_diagnostic *diagnostic) {
  graphion_insn *out;
  if (program == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (program->program_len >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return fail(diagnostic, line, 1U, "bytecode capacity exceeded", GINT_ERR_CAPACITY);
  }
  out = &program->program[program->program_len++];
  out->op = (uint8_t)op;
  out->a = a;
  out->b = b;
  out->imm = imm;
  return GINT_OK;
}

static int parse_identifier_token(const char **cursor,
                                  char *buffer,
                                  size_t buffer_size,
                                  unsigned int line,
                                  graphion_runtime_diagnostic *diagnostic) {
  const char *start;
  size_t len = 0U;
  if (cursor == NULL || *cursor == NULL || buffer == NULL || buffer_size == 0U) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  skip_spaces(cursor);
  start = *cursor;
  if (!is_ident_start_char(*start)) {
    return fail(diagnostic, line, (unsigned int)(start - *cursor + 1), "expected identifier", GINT_ERR_PARSE);
  }
  while (is_ident_char((*cursor)[0])) {
    if (len + 1U >= buffer_size) {
      return fail(diagnostic, line, 1U, "identifier too long", GINT_ERR_CAPACITY);
    }
    buffer[len++] = **cursor;
    (*cursor)++;
  }
  buffer[len] = '\0';
  return GINT_OK;
}

static int parse_string_literal(graphion_runtime_program *program,
                                const char **cursor,
                                graphion_vm_value *value_out,
                                unsigned int line,
                                graphion_runtime_diagnostic *diagnostic) {
  size_t len = 0U;
  char *dst;
  if (program == NULL || cursor == NULL || *cursor == NULL || value_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (**cursor != '"') {
    return fail(diagnostic, line, 1U, "expected string literal", GINT_ERR_PARSE);
  }
  (*cursor)++;
  if (program->const_count >= GRAPHION_RUNTIME_CONST_MAX) {
    return fail(diagnostic, line, 1U, "too many string literals", GINT_ERR_CAPACITY);
  }
  while (**cursor != '\0' && **cursor != '"') {
    if (**cursor == '\n') {
      return fail(diagnostic, line, 1U, "unterminated string literal", GINT_ERR_PARSE);
    }
    len++;
    (*cursor)++;
  }
  if (**cursor != '"') {
    return fail(diagnostic, line, 1U, "unterminated string literal", GINT_ERR_PARSE);
  }
  (*cursor)++;
  dst = (char *)malloc(len + 1U);
  if (dst == NULL) {
    return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
  }
  memcpy(dst, *cursor - len - 1U, len);
  dst[len] = '\0';
  program->owned_const_strings[program->const_count] = dst;
  value_out->kind = GVM_VALUE_STRING;
  value_out->as.string_value = dst;
  return GINT_OK;
}

static int parse_scalar_literal(graphion_runtime_program *program,
                                const char **cursor,
                                graphion_vm_value *value_out,
                                unsigned int line,
                                graphion_runtime_diagnostic *diagnostic) {
  const char *start;
  char *end = NULL;
  if (cursor == NULL || *cursor == NULL || value_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  skip_spaces(cursor);
  start = *cursor;
  if (*start == '"') {
    return parse_string_literal(program, cursor, value_out, line, diagnostic);
  }
  if (strncmp(start, "true", 4U) == 0 && !is_ident_char(start[4])) {
    value_out->kind = GVM_VALUE_BOOL;
    value_out->as.bool_value = 1;
    *cursor += 4;
    return GINT_OK;
  }
  if (strncmp(start, "false", 5U) == 0 && !is_ident_char(start[5])) {
    value_out->kind = GVM_VALUE_BOOL;
    value_out->as.bool_value = 0;
    *cursor += 5;
    return GINT_OK;
  }
  if (*start == '-' || isdigit((unsigned char)*start)) {
    int saw_dot = 0;
    const char *scan = start;
    if (*scan == '-') {
      scan++;
    }
    while (isdigit((unsigned char)*scan)) {
      scan++;
    }
    if (*scan == '.') {
      saw_dot = 1;
      scan++;
      while (isdigit((unsigned char)*scan)) {
        scan++;
      }
    }
    if (scan == start || (*start == '-' && scan == start + 1 && !isdigit((unsigned char)start[1]))) {
      return fail(diagnostic, line, 1U, "expected scalar literal", GINT_ERR_PARSE);
    }
    if (saw_dot) {
      double as_float;
      as_float = strtod(start, &end);
      if (end != scan) {
        return fail(diagnostic, line, 1U, "invalid float literal", GINT_ERR_PARSE);
      }
      value_out->kind = GVM_VALUE_FLOAT;
      value_out->as.float_value = as_float;
    } else {
      long long as_int;
      as_int = strtoll(start, &end, 10);
      if (end != scan) {
        return fail(diagnostic, line, 1U, "invalid integer literal", GINT_ERR_PARSE);
      }
      value_out->kind = GVM_VALUE_INT;
      value_out->as.int_value = (int64_t)as_int;
    }
    *cursor = scan;
    return GINT_OK;
  }
  return fail(diagnostic, line, 1U, "expected scalar literal", GINT_ERR_PARSE);
}

static int parse_operand(const char **cursor,
                         graphion_runtime_program *program,
                         parsed_operand *operand_out,
                         unsigned int line,
                         graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value literal;
  const char *saved;
  int rc;
  if (cursor == NULL || *cursor == NULL || program == NULL || operand_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  saved = *cursor;
  skip_spaces(cursor);
  if (is_ident_start_char(**cursor)) {
    char name[GRAPHION_RUNTIME_NAME_MAX];
    rc = parse_identifier_token(cursor, name, sizeof(name), line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    if (strcmp(name, "true") == 0 || strcmp(name, "false") == 0) {
      *cursor = saved;
    } else {
      int index;
      index = program_find_global_index(program, name);
      if (index < 0) {
        return fail(diagnostic, line, 1U, "unknown operand", GINT_ERR_UNKNOWN_OPERAND);
      }
      operand_out->kind = OPERAND_GLOBAL;
      operand_out->global_index = (size_t)index;
      operand_out->const_index = 0U;
      return GINT_OK;
    }
  }
  rc = parse_scalar_literal(program, cursor, &literal, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_add_const(program, &literal, line, diagnostic, &operand_out->const_index);
  if (rc != GINT_OK) {
    return rc;
  }
  operand_out->kind = OPERAND_LITERAL;
  operand_out->global_index = 0U;
  return GINT_OK;
}

static int emit_load_operand(graphion_runtime_program *program,
                             const parsed_operand *operand,
                             uint8_t reg,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic) {
  if (program == NULL || operand == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (operand->kind == OPERAND_LITERAL) {
    return program_emit(program, GVM_OP_LOAD_CONST, reg, 0U, (int32_t)operand->const_index, line, diagnostic);
  }
  return program_emit(program, GVM_OP_LOAD_GLOBAL, reg, 0U, (int32_t)operand->global_index, line, diagnostic);
}

static int ensure_expr_in_reg(graphion_runtime_program *program,
                              parsed_expr_result *expr,
                              uint8_t reg,
                              unsigned int line,
                              graphion_runtime_diagnostic *diagnostic) {
  parsed_operand operand;
  if (program == NULL || expr == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (expr->kind == EXPR_RESULT_REG && expr->reg_index == reg) {
    return GINT_OK;
  }
  if (expr->kind == EXPR_RESULT_REG) {
    return program_emit(program, GVM_OP_MOV, reg, expr->reg_index, 0, line, diagnostic);
  }
  operand.kind = expr->kind == EXPR_RESULT_LITERAL ? OPERAND_LITERAL : OPERAND_GLOBAL;
  operand.const_index = expr->const_index;
  operand.global_index = expr->global_index;
  {
    int rc = emit_load_operand(program, &operand, reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  expr->kind = EXPR_RESULT_REG;
  expr->reg_index = reg;
  expr->const_index = 0U;
  expr->global_index = 0U;
  return GINT_OK;
}

static int parse_expression(const char **cursor,
                            graphion_runtime_program *program,
                            parsed_expr_result *result_out,
                            uint8_t base_reg,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic);

static int parse_additive_expression(const char **cursor,
                                     graphion_runtime_program *program,
                                     parsed_expr_result *result_out,
                                     uint8_t base_reg,
                                     unsigned int line,
                                     graphion_runtime_diagnostic *diagnostic);

static void scope_sync_to_program(graphion_runtime_scope *scope, const graphion_runtime_program *program);

static int parse_factor(const char **cursor,
                        graphion_runtime_program *program,
                        parsed_expr_result *result_out,
                        uint8_t base_reg,
                        unsigned int line,
                        graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  int rc;
  skip_spaces(cursor);
  if (strncmp(*cursor, "abs", 3U) == 0 && !is_ident_char((*cursor)[3])) {
    const uint8_t target_reg = base_reg;
    const char *after_name = *cursor + 3;
    skip_spaces(&after_name);
    if (*after_name != '(') {
      return fail(diagnostic, line, 1U, "expected '(' after abs", GINT_ERR_PARSE);
    }
    *cursor = after_name + 1;
    rc = parse_expression(cursor, program, &lhs, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ')') {
      return fail(diagnostic, line, 1U, "expected ')' after abs argument", GINT_ERR_PARSE);
    }
    (*cursor)++;
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_ABS, target_reg, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
  } else if (**cursor == '(') {
    (*cursor)++;
    rc = parse_expression(cursor, program, &lhs, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ')') {
      return fail(diagnostic, line, 1U, "expected ')' after expression", GINT_ERR_PARSE);
    }
    (*cursor)++;
  } else {
    parsed_operand operand;
    rc = parse_operand(cursor, program, &operand, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = operand.kind == OPERAND_LITERAL ? EXPR_RESULT_LITERAL : EXPR_RESULT_GLOBAL;
    lhs.const_index = operand.const_index;
    lhs.global_index = operand.global_index;
    lhs.reg_index = 0U;
  }
  skip_spaces(cursor);
  if ((*cursor)[0] == '*' && (*cursor)[1] == '*') {
    parsed_expr_result rhs;
    const uint8_t target_reg = base_reg;
    const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
    *cursor += 2;
    rc = parse_factor(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_POW, target_reg, scratch_reg, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
  }
  *result_out = lhs;
  return GINT_OK;
}

static int parse_term(const char **cursor,
                      graphion_runtime_program *program,
                      parsed_expr_result *result_out,
                      uint8_t base_reg,
                      unsigned int line,
                      graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  const uint8_t target_reg = base_reg;
  const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
  int rc = parse_factor(cursor, program, &lhs, base_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  for (;;) {
    char op;
    int floor_div = 0;
    parsed_expr_result rhs;
    skip_spaces(cursor);
    op = **cursor;
    if (op == '/' && (*cursor)[1] == '/') {
      floor_div = 1;
    } else if (op != '*' && op != '/' && op != '%') {
      break;
    }
    *cursor += floor_div ? 2 : 1;
    rc = parse_factor(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program,
                      op == '*' ? GVM_OP_MUL :
                      floor_div ? GVM_OP_FLOOR_DIV :
                      op == '/' ? GVM_OP_DIV : GVM_OP_MOD,
                      target_reg,
                      scratch_reg,
                      0,
                      line,
                      diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
  }
  *result_out = lhs;
  return GINT_OK;
}

static int parse_additive_expression(const char **cursor,
                                     graphion_runtime_program *program,
                                     parsed_expr_result *result_out,
                                     uint8_t base_reg,
                                     unsigned int line,
                                     graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  const uint8_t target_reg = base_reg;
  const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
  int rc = parse_term(cursor, program, &lhs, base_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  for (;;) {
    char op;
    parsed_expr_result rhs;
    skip_spaces(cursor);
    op = **cursor;
    if (op != '+' && op != '-') {
      break;
    }
    (*cursor)++;
    rc = parse_term(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program,
                      op == '+' ? GVM_OP_ADD : GVM_OP_SUB,
                      target_reg,
                      scratch_reg,
                      0,
                      line,
                      diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
  }
  *result_out = lhs;
  return GINT_OK;
}

static int parse_expression(const char **cursor,
                            graphion_runtime_program *program,
                            parsed_expr_result *result_out,
                            uint8_t base_reg,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  const uint8_t target_reg = base_reg;
  const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
  int rc = parse_additive_expression(cursor, program, &lhs, base_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  for (;;) {
    parsed_expr_result rhs;
    graphion_opcode cmp_op;
    skip_spaces(cursor);
    if ((*cursor)[0] == '=' && (*cursor)[1] == '=') {
      cmp_op = GVM_OP_EQ;
      *cursor += 2;
    } else if ((*cursor)[0] == '!' && (*cursor)[1] == '=') {
      cmp_op = GVM_OP_NE;
      *cursor += 2;
    } else if ((*cursor)[0] == '<' && (*cursor)[1] == '=') {
      cmp_op = GVM_OP_LE;
      *cursor += 2;
    } else if ((*cursor)[0] == '<' && (*cursor)[1] != '=') {
      cmp_op = GVM_OP_LT;
      *cursor += 1;
    } else if ((*cursor)[0] == '>' && (*cursor)[1] != '=') {
      cmp_op = GVM_OP_GT;
      *cursor += 1;
    } else {
      break;
    }
    rc = parse_additive_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, cmp_op, target_reg, scratch_reg, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
  }
  *result_out = lhs;
  return GINT_OK;
}

static int parse_assignment(const char *line_text,
                            graphion_runtime_program *program,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = line_text;
  char target[GRAPHION_RUNTIME_NAME_MAX];
  parsed_expr_result expr;
  size_t target_index;
  char assign_op = '=';
  int power_assign = 0;
  int floor_div_assign = 0;
  int rc;

  rc = parse_identifier_token(&cursor, target, sizeof(target), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  if (is_reserved_name(target)) {
    return fail(diagnostic, line, 1U, "reserved name cannot be assigned", GINT_ERR_RESERVED_NAME);
  }
  skip_spaces(&cursor);
  if (cursor[0] == '*' && cursor[1] == '*' && cursor[2] == '=') {
    assign_op = '*';
    power_assign = 1;
    cursor += 3;
  } else if (cursor[0] == '/' && cursor[1] == '/' && cursor[2] == '=') {
    assign_op = '/';
    floor_div_assign = 1;
    cursor += 3;
  } else if ((*cursor == '+' || *cursor == '-' || *cursor == '*' || *cursor == '/' || *cursor == '%') &&
      cursor[1] == '=') {
    assign_op = *cursor;
    cursor += 2;
  } else if (*cursor == '=') {
    cursor++;
  } else {
    return fail(diagnostic, line, 1U, "expected '='", GINT_ERR_PARSE);
  }
  if (assign_op == '=') {
    rc = program_find_or_add_global(program, target, line, diagnostic, &target_index);
    if (rc != GINT_OK) {
      return rc;
    }
  } else {
    int existing = program_find_global_index(program, target);
    if (existing < 0) {
      return fail(diagnostic, line, 1U, "unknown variable", GINT_ERR_UNKNOWN_VARIABLE);
    }
    target_index = (size_t)existing;
  }
  rc = parse_expression(&cursor, program, &expr, 0U, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unsupported assignment expression", GINT_ERR_PARSE);
  }
  if (assign_op != '=') {
    rc = program_emit(program, GVM_OP_LOAD_GLOBAL, 0U, 0U, (int32_t)target_index, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &expr, 1U, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program,
                      power_assign ? GVM_OP_POW :
                      floor_div_assign ? GVM_OP_FLOOR_DIV :
                      assign_op == '+' ? GVM_OP_ADD :
                      assign_op == '-' ? GVM_OP_SUB :
                      assign_op == '*' ? GVM_OP_MUL :
                      assign_op == '/' ? GVM_OP_DIV : GVM_OP_MOD,
                      0U,
                      1U,
                      0,
                      line,
                      diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, (int32_t)target_index, line, diagnostic);
  }
  if (expr.kind == EXPR_RESULT_LITERAL) {
    return program_emit(
        program, GVM_OP_STORE_CONST_GLOBAL, 0U, (uint8_t)target_index, (int32_t)expr.const_index, line, diagnostic);
  }
  if (expr.kind == EXPR_RESULT_GLOBAL) {
    return program_emit(program, GVM_OP_COPY_GLOBAL, 0U, (uint8_t)target_index, (int32_t)expr.global_index, line, diagnostic);
  }
  return program_emit(program, GVM_OP_STORE_GLOBAL, expr.reg_index, 0U, (int32_t)target_index, line, diagnostic);
}

static int parse_print(const char *line_text,
                       const graphion_runtime_scope *scope,
                       graphion_runtime_program *program,
                       unsigned int line,
                       graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = line_text;
  parsed_expr_result expr;
  int rc;
  skip_spaces(&cursor);
  if (strncmp(cursor, "print", 5U) != 0 || is_ident_char(cursor[5])) {
    return fail(diagnostic, line, 1U, "expected 'print'", GINT_ERR_PARSE);
  }
  cursor += 5;
  skip_spaces(&cursor);
  if (*cursor != '(') {
    return fail(diagnostic, line, 1U, "expected '(' after print", GINT_ERR_PARSE);
  }
  cursor++;
  {
    const char *scan = cursor;
    int depth = 0;
    int in_string = 0;
    int has_concat = 0;
    int has_stringish = 0;
    const char *segment_start = cursor;
    while (*scan != '\0') {
      if (in_string) {
        if (*scan == '"') {
          in_string = 0;
        }
        scan++;
        continue;
      }
      if (*scan == '"') {
        in_string = 1;
        scan++;
        continue;
      }
      if (*scan == '(') {
        depth++;
        scan++;
        continue;
      }
      if (*scan == ')') {
        if (depth == 0) {
          break;
        }
        depth--;
        scan++;
        continue;
      }
      if (depth == 0 && *scan == '+') {
        const char *trim_start = segment_start;
        const char *trim_end = scan;
        while (trim_start < trim_end && (*trim_start == ' ' || *trim_start == '\t' || *trim_start == '\r')) {
          trim_start++;
        }
        while (trim_end > trim_start && (trim_end[-1] == ' ' || trim_end[-1] == '\t' || trim_end[-1] == '\r')) {
          trim_end--;
        }
        if (trim_start < trim_end) {
          if (*trim_start == '"') {
            has_stringish = 1;
          } else {
            size_t len = (size_t)(trim_end - trim_start);
            if (scope != NULL && len < GRAPHION_RUNTIME_NAME_MAX) {
              char name[GRAPHION_RUNTIME_NAME_MAX];
              memcpy(name, trim_start, len);
              name[len] = '\0';
              if (scope_find_index(scope, name) >= 0) {
                const graphion_runtime_value *value = graphion_runtime_scope_find(scope, name);
                if (value != NULL && value->kind == GVM_VALUE_STRING) {
                  has_stringish = 1;
                }
              }
            }
          }
        }
        has_concat = 1;
        segment_start = scan + 1;
      }
      scan++;
    }
    if (has_concat) {
      const char *trim_start = segment_start;
      const char *trim_end = scan;
      while (trim_start < trim_end && (*trim_start == ' ' || *trim_start == '\t' || *trim_start == '\r')) {
        trim_start++;
      }
      while (trim_end > trim_start && (trim_end[-1] == ' ' || trim_end[-1] == '\t' || trim_end[-1] == '\r')) {
        trim_end--;
      }
      if (trim_start < trim_end) {
        if (*trim_start == '"') {
          has_stringish = 1;
        } else {
          size_t len = (size_t)(trim_end - trim_start);
          if (scope != NULL && len < GRAPHION_RUNTIME_NAME_MAX) {
            char name[GRAPHION_RUNTIME_NAME_MAX];
            memcpy(name, trim_start, len);
            name[len] = '\0';
            if (scope_find_index(scope, name) >= 0) {
              const graphion_runtime_value *value = graphion_runtime_scope_find(scope, name);
              if (value != NULL && value->kind == GVM_VALUE_STRING) {
                has_stringish = 1;
              }
            }
          }
        }
      }
    }
    if (has_concat && has_stringish) {
      const char *part_cursor = cursor;
      const char *part_start = cursor;
      for (;;) {
        int depth2 = 0;
        int in_string2 = 0;
        while (*part_cursor != '\0') {
          if (in_string2) {
            if (*part_cursor == '"') {
              in_string2 = 0;
            }
            part_cursor++;
            continue;
          }
          if (*part_cursor == '"') {
            in_string2 = 1;
            part_cursor++;
            continue;
          }
          if (*part_cursor == '(') {
            depth2++;
            part_cursor++;
            continue;
          }
          if (*part_cursor == ')') {
            if (depth2 == 0) {
              break;
            }
            depth2--;
            part_cursor++;
            continue;
          }
          if (depth2 == 0 && *part_cursor == '+') {
            break;
          }
          part_cursor++;
        }
        {
          char segment[512];
          size_t len = (size_t)(part_cursor - part_start);
          parsed_expr_result part_expr;
          const char *segment_cursor = segment;
          if (len >= sizeof(segment)) {
            return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
          }
          memcpy(segment, part_start, len);
          segment[len] = '\0';
          rc = parse_expression(&segment_cursor, program, &part_expr, 0U, line, diagnostic);
          if (rc != GINT_OK) {
            return rc;
          }
          skip_spaces(&segment_cursor);
          if (*segment_cursor != '\0') {
            return fail(diagnostic, line, 1U, "unexpected trailing tokens after print", GINT_ERR_PARSE);
          }
          if (part_expr.kind == EXPR_RESULT_LITERAL) {
            rc = program_emit(program, GVM_OP_PRINT_CONST_PART, 0U, 0U, (int32_t)part_expr.const_index, line, diagnostic);
          } else if (part_expr.kind == EXPR_RESULT_GLOBAL) {
            rc = program_emit(program, GVM_OP_PRINT_GLOBAL_PART, 0U, 0U, (int32_t)part_expr.global_index, line, diagnostic);
          } else {
            rc = program_emit(program, GVM_OP_PRINT_REG_PART, part_expr.reg_index, 0U, 0, line, diagnostic);
          }
          if (rc != GINT_OK) {
            return rc;
          }
        }
        if (*part_cursor == ')') {
          cursor = part_cursor;
          break;
        }
        part_cursor++;
        part_start = part_cursor;
      }
      if (*cursor != ')') {
        return fail(diagnostic, line, 1U, "expected ')' after print argument", GINT_ERR_PARSE);
      }
      cursor++;
      skip_spaces(&cursor);
      if (*cursor != '\0') {
        return fail(diagnostic, line, 1U, "unexpected trailing tokens after print", GINT_ERR_PARSE);
      }
      return program_emit(program, GVM_OP_PRINT_NEWLINE, 0U, 0U, 0, line, diagnostic);
    }
  }
  rc = parse_expression(&cursor, program, &expr, 0U, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != ')') {
    return fail(diagnostic, line, 1U, "expected ')' after print argument", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after print", GINT_ERR_PARSE);
  }
  if (expr.kind == EXPR_RESULT_LITERAL) {
    return program_emit(program, GVM_OP_PRINT_CONST, 0U, 0U, (int32_t)expr.const_index, line, diagnostic);
  }
  if (expr.kind == EXPR_RESULT_GLOBAL) {
    return program_emit(program, GVM_OP_PRINT_GLOBAL, 0U, 0U, (int32_t)expr.global_index, line, diagnostic);
  }
  return program_emit(program, GVM_OP_PRINT_REG, expr.reg_index, 0U, 0, line, diagnostic);
}

static void seed_program_from_scope(graphion_runtime_program *program, const graphion_runtime_scope *scope) {
  size_t i;
  if (program == NULL || scope == NULL) {
    return;
  }
  graphion_runtime_program_init(program);
  program->global_count = scope->global_count;
  for (i = 0U; i < scope->global_count; ++i) {
    copy_name(program->global_names[i], scope->global_names[i]);
  }
}

static int parse_statement_line(const char *line_text,
                                const graphion_runtime_scope *scope,
                                graphion_runtime_program *program,
                                unsigned int line,
                                graphion_runtime_diagnostic *diagnostic) {
  const char *line_cursor = line_text;
  int rc;
  skip_spaces(&line_cursor);
  if (*line_cursor == '\0') {
    return GINT_OK;
  }
  if (strncmp(line_cursor, "print", 5U) == 0 && !is_ident_char(line_cursor[5])) {
    rc = parse_print(line_cursor, scope, program, line, diagnostic);
  } else {
    rc = parse_assignment(line_cursor, program, line, diagnostic);
  }
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_HALT, 0U, 0U, 0, line, diagnostic);
}

static int line_is_blank(const runtime_source_line *line) {
  const char *cursor = line->text;
  skip_spaces(&cursor);
  return *cursor == '\0';
}

static const char *line_content(const runtime_source_line *line) {
  const char *cursor = line->text;
  skip_spaces(&cursor);
  return cursor;
}

static int line_starts_with_keyword(const runtime_source_line *line, const char *keyword) {
  const char *cursor = line_content(line);
  const size_t len = strlen(keyword);
  return strncmp(cursor, keyword, len) == 0 && !is_ident_char(cursor[len]);
}

static int line_keyword_is_assignment_like(const runtime_source_line *line, const char *keyword) {
  const char *cursor = line_content(line);
  const size_t len = strlen(keyword);
  if (strncmp(cursor, keyword, len) != 0 || is_ident_char(cursor[len])) {
    return 0;
  }
  cursor += len;
  skip_spaces(&cursor);
  if (*cursor == '=') {
    return 1;
  }
  if ((cursor[0] == '+' || cursor[0] == '-' || cursor[0] == '*' || cursor[0] == '/' || cursor[0] == '%') &&
      cursor[1] == '=') {
    return 1;
  }
  if (cursor[0] == '*' && cursor[1] == '*' && cursor[2] == '=') {
    return 1;
  }
  if (cursor[0] == '/' && cursor[1] == '/' && cursor[2] == '=') {
    return 1;
  }
  return 0;
}

static int line_is_if_clause(const runtime_source_line *line) {
  return line_starts_with_keyword(line, "if") && !line_keyword_is_assignment_like(line, "if");
}

static int line_is_elif_clause(const runtime_source_line *line) {
  return line_starts_with_keyword(line, "elif") && !line_keyword_is_assignment_like(line, "elif");
}

static int line_is_else_clause(const runtime_source_line *line) {
  return line_starts_with_keyword(line, "else") && !line_keyword_is_assignment_like(line, "else");
}

static int copy_source_without_comments(const char *source,
                                        char **clean_out,
                                        graphion_runtime_diagnostic *diagnostic) {
  size_t len;
  char *clean;
  size_t read_index = 0U;
  size_t write_index = 0U;
  unsigned int line = 1U;
  unsigned int block_comment_line = 0U;
  int in_string = 0;
  int in_block_comment = 0;
  int in_line_comment = 0;
  if (source == NULL || clean_out == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  len = strlen(source);
  clean = (char *)malloc(len + 1U);
  if (clean == NULL) {
    return fail(diagnostic, 1U, 1U, "out of memory", GINT_ERR_CAPACITY);
  }
  while (source[read_index] != '\0') {
    const char ch = source[read_index];
    const char next = source[read_index + 1U];
    if (in_block_comment) {
      if (ch == '*' && next == '/') {
        in_block_comment = 0;
        read_index += 2U;
        continue;
      }
      if (ch == '\n') {
        clean[write_index++] = '\n';
        line++;
      }
      read_index++;
      continue;
    }
    if (in_line_comment) {
      if (ch == '\n') {
        clean[write_index++] = '\n';
        line++;
        in_line_comment = 0;
      }
      read_index++;
      continue;
    }
    if (in_string) {
      clean[write_index++] = ch;
      if (ch == '"') {
        in_string = 0;
      }
      if (ch == '\n') {
        line++;
      }
      read_index++;
      continue;
    }
    if (ch == '"') {
      in_string = 1;
      clean[write_index++] = ch;
      read_index++;
      continue;
    }
    if (ch == '#') {
      in_line_comment = 1;
      read_index++;
      continue;
    }
    if (ch == '/' && next == '*') {
      in_block_comment = 1;
      block_comment_line = line;
      read_index += 2U;
      continue;
    }
    clean[write_index++] = ch;
    if (ch == '\n') {
      line++;
    }
    read_index++;
  }
  if (in_block_comment) {
    free(clean);
    return fail(diagnostic, block_comment_line, 1U, "unterminated block comment", GINT_ERR_PARSE);
  }
  clean[write_index] = '\0';
  *clean_out = clean;
  return GINT_OK;
}

static int split_source_lines(const char *source,
                              runtime_source_line *lines,
                              size_t capacity,
                              size_t *count_out,
                              graphion_runtime_diagnostic *diagnostic) {
  const char *line_start;
  const char *cursor;
  char *clean_source = NULL;
  unsigned int line_number = 1U;
  size_t count = 0U;
  int rc;
  if (source == NULL || lines == NULL || count_out == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  rc = copy_source_without_comments(source, &clean_source, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  line_start = clean_source;
  cursor = clean_source;
  for (;;) {
    if (*cursor == '\n' || *cursor == '\0') {
      size_t len = (size_t)(cursor - line_start);
      size_t indent = 0U;
      if (count >= capacity) {
        free(clean_source);
        return fail(diagnostic, line_number, 1U, "too many source lines", GINT_ERR_CAPACITY);
      }
      if (len >= sizeof(lines[count].text)) {
        free(clean_source);
        return fail(diagnostic, line_number, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      memcpy(lines[count].text, line_start, len);
      lines[count].text[len] = '\0';
      while (lines[count].text[indent] == ' ' || lines[count].text[indent] == '\t') {
        indent++;
      }
      lines[count].line = line_number;
      lines[count].indent = (unsigned int)indent;
      count++;
      if (*cursor == '\0') {
        break;
      }
      cursor++;
      line_number++;
      line_start = cursor;
      continue;
    }
    cursor++;
  }
  free(clean_source);
  *count_out = count;
  return GINT_OK;
}

static size_t find_next_nonblank_line(const runtime_source_line *lines, size_t count, size_t start) {
  size_t i;
  for (i = start; i < count; ++i) {
    if (!line_is_blank(&lines[i])) {
      return i;
    }
  }
  return count;
}

static size_t scan_block_end(const runtime_source_line *lines, size_t count, size_t start, unsigned int block_indent) {
  size_t i;
  for (i = start; i < count; ++i) {
    if (line_is_blank(&lines[i])) {
      continue;
    }
    if (lines[i].indent < block_indent) {
      break;
    }
  }
  return i;
}

static int parse_control_condition_span(const char *cursor,
                                        const char *keyword,
                                        const char **cond_start_out,
                                        const char **cond_end_out,
                                        unsigned int line,
                                        graphion_runtime_diagnostic *diagnostic) {
  const size_t keyword_len = strlen(keyword);
  const char *scan;
  int depth = 0;
  int in_string = 0;
  if (strncmp(cursor, keyword, keyword_len) != 0 || is_ident_char(cursor[keyword_len])) {
    return fail(diagnostic, line, 1U, "invalid conditional header", GINT_ERR_PARSE);
  }
  cursor += keyword_len;
  skip_spaces(&cursor);
  if (*cursor == '\0') {
    return fail(diagnostic, line, 1U, strcmp(keyword, "if") == 0 ? "expected condition after if" : "expected condition after elif", GINT_ERR_PARSE);
  }
  *cond_start_out = cursor;
  scan = cursor;
  while (*scan != '\0') {
    if (in_string) {
      if (*scan == '"') {
        in_string = 0;
      }
      scan++;
      continue;
    }
    if (*scan == '"') {
      in_string = 1;
      scan++;
      continue;
    }
    if (*scan == '(') {
      depth++;
      scan++;
      continue;
    }
    if (*scan == ')') {
      if (depth > 0) {
        depth--;
      }
      scan++;
      continue;
    }
    if (*scan == ':' && depth == 0) {
      const char *tail = scan + 1;
      const char *cond_end = scan;
      while (cond_end > *cond_start_out && (cond_end[-1] == ' ' || cond_end[-1] == '\t' || cond_end[-1] == '\r')) {
        cond_end--;
      }
      skip_spaces(&tail);
      if (*tail != '\0') {
        return fail(diagnostic, line, 1U, "unexpected trailing tokens after condition", GINT_ERR_PARSE);
      }
      if (cond_end == *cond_start_out) {
        return fail(diagnostic, line, 1U, strcmp(keyword, "if") == 0 ? "expected condition after if" : "expected condition after elif", GINT_ERR_PARSE);
      }
      *cond_end_out = cond_end;
      return GINT_OK;
    }
    scan++;
  }
  return fail(diagnostic, line, 1U, strcmp(keyword, "if") == 0 ? "expected ':' after if condition" : "expected ':' after elif condition", GINT_ERR_PARSE);
}

static int parse_else_header(const char *cursor,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic) {
  if (strncmp(cursor, "else", 4U) != 0 || is_ident_char(cursor[4])) {
    return fail(diagnostic, line, 1U, "invalid else header", GINT_ERR_PARSE);
  }
  cursor += 4;
  skip_spaces(&cursor);
  if (*cursor != ':') {
    return fail(diagnostic, line, 1U, "expected ':' after else", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after else", GINT_ERR_PARSE);
  }
  return GINT_OK;
}

static int execute_condition_program(const graphion_runtime_program *program,
                                     graphion_runtime_scope *scope,
                                     uint8_t reg_index,
                                     unsigned int line,
                                     graphion_runtime_diagnostic *diagnostic,
                                     graphion_vm_value *value_out) {
  graphion_vm vm;
  int rc;
  if (program == NULL || scope == NULL || value_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  scope_sync_to_program(scope, program);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, program->const_pool, program->const_count);
  graphion_vm_bind_globals(&vm, scope->globals, scope->global_count);
  graphion_vm_bind_global_string_owners(&vm, scope->owned_string_values, scope->global_count);
  rc = graphion_vm_load(&vm, program->program, program->program_len);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return fail(diagnostic, line, 1U, "failed to load VM program", GINT_ERR_PARSE);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    if (rc == GVM_ERR_DIVIDE_BY_ZERO) {
      return fail(diagnostic, line, 1U, "division by zero", GINT_ERR_RUN);
    }
    if (rc == GVM_ERR_TYPE_MISMATCH) {
      return fail(diagnostic, line, 1U, "incompatible operand types", GINT_ERR_RUN);
    }
    return fail(diagnostic, line, 1U, "failed to execute VM program", GINT_ERR_RUN);
  }
  *value_out = vm.regs[reg_index];
  if (value_out->kind == GVM_VALUE_STRING) {
    value_out->as.string_value = NULL;
  }
  graphion_vm_dispose(&vm);
  return GINT_OK;
}

static int evaluate_condition_text(const char *condition_text,
                                   size_t condition_len,
                                   graphion_runtime_scope *scope,
                                   unsigned int line,
                                   graphion_runtime_diagnostic *diagnostic,
                                   int *result_out) {
  char condition_buffer[512];
  const char *cursor = condition_buffer;
  parsed_expr_result expr;
  graphion_runtime_program program;
  graphion_vm_value value;
  int rc;
  if (scope == NULL || condition_text == NULL || result_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (condition_len >= sizeof(condition_buffer)) {
    return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
  }
  memcpy(condition_buffer, condition_text, condition_len);
  condition_buffer[condition_len] = '\0';
  seed_program_from_scope(&program, scope);
  rc = parse_expression(&cursor, &program, &expr, 0U, line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    graphion_runtime_program_dispose(&program);
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after condition", GINT_ERR_PARSE);
  }
  if (expr.kind == EXPR_RESULT_LITERAL) {
    value = program.const_pool[expr.const_index];
  } else if (expr.kind == EXPR_RESULT_GLOBAL) {
    value = scope->globals[expr.global_index];
  } else {
    rc = program_emit(&program, GVM_OP_HALT, 0U, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      graphion_runtime_program_dispose(&program);
      return rc;
    }
    rc = execute_condition_program(&program, scope, expr.reg_index, line, diagnostic, &value);
    graphion_runtime_program_dispose(&program);
    if (rc != GINT_OK) {
      return rc;
    }
    if (value.kind == GVM_VALUE_BOOL) {
      *result_out = value.as.bool_value != 0;
      return GINT_OK;
    }
    if (value.kind == GVM_VALUE_INT && (value.as.int_value == 0 || value.as.int_value == 1)) {
      *result_out = value.as.int_value != 0;
      return GINT_OK;
    }
    return fail(diagnostic, line, 1U, "if condition must be boolean or 0/1", GINT_ERR_RUN);
  }
  graphion_runtime_program_dispose(&program);
  if (value.kind == GVM_VALUE_BOOL) {
    *result_out = value.as.bool_value != 0;
    return GINT_OK;
  }
  if (value.kind == GVM_VALUE_INT && (value.as.int_value == 0 || value.as.int_value == 1)) {
    *result_out = value.as.int_value != 0;
    return GINT_OK;
  }
  return fail(diagnostic, line, 1U, "if condition must be boolean or 0/1", GINT_ERR_RUN);
}

static int execute_statement_source_line(const runtime_source_line *line,
                                         graphion_runtime_scope *scope,
                                         graphion_runtime_diagnostic *diagnostic,
                                         FILE *output) {
  graphion_runtime_program program;
  int rc;
  seed_program_from_scope(&program, scope);
  rc = parse_statement_line(line_content(line), scope, &program, line->line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  if (program.program_len > 0U) {
    rc = graphion_execute_program(&program, scope, diagnostic, output);
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  graphion_runtime_program_dispose(&program);
  return GINT_OK;
}

static int execute_block(const runtime_source_line *lines,
                         size_t count,
                         size_t *index,
                         unsigned int block_indent,
                         graphion_runtime_scope *scope,
                         graphion_runtime_diagnostic *diagnostic,
                         FILE *output);

static int execute_if_chain(const runtime_source_line *lines,
                            size_t count,
                            size_t *index,
                            unsigned int current_indent,
                            graphion_runtime_scope *scope,
                            graphion_runtime_diagnostic *diagnostic,
                            FILE *output) {
  size_t clause_index = *index;
  int branch_taken = 0;
  int seen_else = 0;
  int first_clause = 1;
  while (clause_index < count) {
    const runtime_source_line *clause_line;
    const char *cursor;
    size_t body_start;
    size_t body_end;
    unsigned int body_indent;
    int is_else_clause;
    if (line_is_blank(&lines[clause_index])) {
      clause_index++;
      continue;
    }
    if (lines[clause_index].indent != current_indent) {
      break;
    }
    clause_line = &lines[clause_index];
    cursor = line_content(clause_line);
    is_else_clause = line_is_else_clause(clause_line);
    if (!line_is_if_clause(clause_line) && !line_is_elif_clause(clause_line) && !is_else_clause) {
      break;
    }
    if (!first_clause && line_is_if_clause(clause_line)) {
      break;
    }
    if (seen_else && line_is_if_clause(clause_line)) {
      break;
    }
    if (seen_else && (line_is_elif_clause(clause_line) || is_else_clause)) {
      return fail(diagnostic, clause_line->line, 1U, "else must be last in if chain", GINT_ERR_PARSE);
    }
    body_start = find_next_nonblank_line(lines, count, clause_index + 1U);
    if (body_start >= count || lines[body_start].indent <= current_indent) {
      return fail(diagnostic,
                  clause_line->line,
                  1U,
                  is_else_clause ? "expected indented block after else" :
                  line_is_elif_clause(clause_line) ? "expected indented block after elif" :
                  "expected indented block after if",
                  GINT_ERR_PARSE);
    }
    body_indent = lines[body_start].indent;
    body_end = scan_block_end(lines, count, body_start, body_indent);
    if (is_else_clause) {
      int rc = parse_else_header(cursor, clause_line->line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      seen_else = 1;
      if (!branch_taken) {
        size_t exec_index = body_start;
        rc = execute_block(lines, count, &exec_index, body_indent, scope, diagnostic, output);
        if (rc != GINT_OK) {
          return rc;
        }
        body_end = exec_index;
        branch_taken = 1;
      }
    } else {
      const char *cond_start = NULL;
      const char *cond_end = NULL;
      int rc = parse_control_condition_span(cursor,
                                            line_is_elif_clause(clause_line) ? "elif" : "if",
                                            &cond_start,
                                            &cond_end,
                                            clause_line->line,
                                            diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      if (!branch_taken) {
        int condition_true = 0;
        rc = evaluate_condition_text(cond_start,
                                     (size_t)(cond_end - cond_start),
                                     scope,
                                     clause_line->line,
                                     diagnostic,
                                     &condition_true);
        if (rc != GINT_OK) {
          return rc;
        }
        if (condition_true) {
          size_t exec_index = body_start;
          rc = execute_block(lines, count, &exec_index, body_indent, scope, diagnostic, output);
          if (rc != GINT_OK) {
            return rc;
          }
          body_end = exec_index;
          branch_taken = 1;
        }
      }
    }
    clause_index = body_end;
    first_clause = 0;
  }
  *index = clause_index;
  return GINT_OK;
}

static int execute_block(const runtime_source_line *lines,
                         size_t count,
                         size_t *index,
                         unsigned int block_indent,
                         graphion_runtime_scope *scope,
                         graphion_runtime_diagnostic *diagnostic,
                         FILE *output) {
  size_t i = *index;
  while (i < count) {
    if (line_is_blank(&lines[i])) {
      i++;
      continue;
    }
    if (lines[i].indent < block_indent) {
      break;
    }
    if (lines[i].indent > block_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    if (line_is_elif_clause(&lines[i])) {
      return fail(diagnostic, lines[i].line, 1U, "elif without matching if", GINT_ERR_PARSE);
    }
    if (line_is_else_clause(&lines[i])) {
      return fail(diagnostic, lines[i].line, 1U, "else without matching if", GINT_ERR_PARSE);
    }
    if (line_is_if_clause(&lines[i])) {
      int rc = execute_if_chain(lines, count, &i, block_indent, scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
      continue;
    }
    {
      int rc = execute_statement_source_line(&lines[i], scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
    }
    i++;
  }
  *index = i;
  return GINT_OK;
}

void graphion_runtime_scope_init(graphion_runtime_scope *scope) {
  size_t i;
  if (scope == NULL) {
    return;
  }
  scope->global_count = 0U;
  for (i = 0U; i < GRAPHION_RUNTIME_BINDING_MAX; ++i) {
    scope->global_names[i][0] = '\0';
    scope->owned_string_values[i] = NULL;
    vm_value_set_none(&scope->globals[i]);
  }
}

void graphion_runtime_scope_dispose(graphion_runtime_scope *scope) {
  size_t i;
  if (scope == NULL) {
    return;
  }
  for (i = 0U; i < GRAPHION_RUNTIME_BINDING_MAX; ++i) {
    runtime_free_string(&scope->owned_string_values[i]);
    scope->global_names[i][0] = '\0';
    vm_value_set_none(&scope->globals[i]);
  }
  scope->global_count = 0U;
}

const graphion_runtime_value *graphion_runtime_scope_find(const graphion_runtime_scope *scope,
                                                          const char *name) {
  int index = scope_find_index(scope, name);
  if (index < 0) {
    return NULL;
  }
  return &scope->globals[index];
}

void graphion_runtime_program_init(graphion_runtime_program *program) {
  size_t i;
  if (program == NULL) {
    return;
  }
  program->global_count = 0U;
  program->const_count = 0U;
  program->program_len = 0U;
  for (i = 0U; i < GRAPHION_RUNTIME_BINDING_MAX; ++i) {
    program->global_names[i][0] = '\0';
  }
  for (i = 0U; i < GRAPHION_RUNTIME_CONST_MAX; ++i) {
    program->owned_const_strings[i] = NULL;
    vm_value_set_none(&program->const_pool[i]);
  }
  memset(program->program, 0, sizeof(program->program));
}

void graphion_runtime_program_dispose(graphion_runtime_program *program) {
  size_t i;
  if (program == NULL) {
    return;
  }
  for (i = 0U; i < GRAPHION_RUNTIME_CONST_MAX; ++i) {
    runtime_free_string(&program->owned_const_strings[i]);
    vm_value_set_none(&program->const_pool[i]);
  }
  for (i = 0U; i < GRAPHION_RUNTIME_BINDING_MAX; ++i) {
    program->global_names[i][0] = '\0';
  }
  program->global_count = 0U;
  program->const_count = 0U;
  program->program_len = 0U;
  memset(program->program, 0, sizeof(program->program));
}

int graphion_prepare_source(const char *source,
                            graphion_runtime_program *program,
                            graphion_runtime_diagnostic *diagnostic) {
  const char *line_start;
  const char *cursor;
  char *clean_source = NULL;
  unsigned int line = 1U;
  int rc;
  if (source == NULL || program == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  clear_diagnostic(diagnostic);
  graphion_runtime_program_init(program);
  rc = copy_source_without_comments(source, &clean_source, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  line_start = clean_source;
  cursor = clean_source;
  for (;;) {
    if (*cursor == '\n' || *cursor == '\0') {
      char line_buffer[512];
      size_t len = (size_t)(cursor - line_start);
      const char *line_cursor = line_buffer;
      if (len >= sizeof(line_buffer)) {
        free(clean_source);
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      memcpy(line_buffer, line_start, len);
      line_buffer[len] = '\0';
      skip_spaces(&line_cursor);
      if (*line_cursor != '\0') {
        if (strncmp(line_cursor, "print", 5U) == 0 && !is_ident_char(line_cursor[5])) {
          rc = parse_print(line_cursor, NULL, program, line, diagnostic);
        } else {
          rc = parse_assignment(line_cursor, program, line, diagnostic);
        }
        if (rc != GINT_OK) {
          free(clean_source);
          return rc;
        }
      }
      if (*cursor == '\0') {
        break;
      }
      cursor++;
      line++;
      line_start = cursor;
      continue;
    }
    cursor++;
  }
  rc = program_emit(program, GVM_OP_HALT, 0U, 0U, 0, line, diagnostic);
  if (rc != GINT_OK) {
    free(clean_source);
    return rc;
  }
  free(clean_source);
  return GINT_OK;
}

static void scope_sync_to_program(graphion_runtime_scope *scope, const graphion_runtime_program *program) {
  size_t i;
  for (i = scope->global_count; i < program->global_count; ++i) {
    copy_name(scope->global_names[i], program->global_names[i]);
    vm_value_set_none(&scope->globals[i]);
  }
  scope->global_count = program->global_count;
}

int graphion_execute_prepared_program_with_sink(const graphion_runtime_program *program,
                                                graphion_runtime_scope *scope,
                                                graphion_runtime_diagnostic *diagnostic,
                                                const graphion_output_sink *output) {
  graphion_vm vm;
  int rc;
  if (program == NULL || scope == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  clear_diagnostic(diagnostic);
  scope_sync_to_program(scope, program);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, program->const_pool, program->const_count);
  graphion_vm_bind_globals(&vm, scope->globals, scope->global_count);
  graphion_vm_bind_global_string_owners(&vm, scope->owned_string_values, scope->global_count);
  if (output != NULL) {
    graphion_vm_bind_output_sink(&vm, output);
  }
  rc = graphion_vm_load(&vm, program->program, program->program_len);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return fail(diagnostic, 1U, 1U, "failed to load VM program", GINT_ERR_PARSE);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    if (rc == GVM_ERR_DIVIDE_BY_ZERO) {
      return fail(diagnostic, 1U, 1U, "division by zero", GINT_ERR_RUN);
    }
    if (rc == GVM_ERR_TYPE_MISMATCH) {
      return fail(diagnostic, 1U, 1U, "incompatible operand types", GINT_ERR_RUN);
    }
    return fail(diagnostic, 1U, 1U, "failed to execute VM program", GINT_ERR_RUN);
  }
  graphion_vm_dispose(&vm);
  return GINT_OK;
}

int graphion_execute_program(const graphion_runtime_program *program,
                             graphion_runtime_scope *scope,
                             graphion_runtime_diagnostic *diagnostic,
                             FILE *output) {
  graphion_output_sink sink;
  graphion_output_sink_from_file(&sink, output);
  return graphion_execute_prepared_program_with_sink(program, scope, diagnostic, &sink);
}

int graphion_interpret_source_with_output(const char *source,
                                          graphion_runtime_scope *scope,
                                          graphion_runtime_diagnostic *diagnostic,
                                          FILE *output) {
  runtime_source_line lines[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t line_count = 0U;
  size_t index = 0U;
  int rc;
  if (source == NULL || scope == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  clear_diagnostic(diagnostic);
  rc = split_source_lines(source, lines, GRAPHION_RUNTIME_PROGRAM_MAX, &line_count, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = execute_block(lines, line_count, &index, 0U, scope, diagnostic, output);
  if (rc != GINT_OK) {
    return rc;
  }
  while (index < line_count) {
    if (!line_is_blank(&lines[index])) {
      return fail(diagnostic, lines[index].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    index++;
  }
  return GINT_OK;
}

int graphion_interpret_source(const char *source,
                              graphion_runtime_scope *scope,
                              graphion_runtime_diagnostic *diagnostic) {
  return graphion_interpret_source_with_output(source, scope, diagnostic, stdout);
}

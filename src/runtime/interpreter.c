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

typedef struct {
  graphion_vm_value value;
  char *owned_string;
} runtime_match_case_value;

static void clear_diagnostic(graphion_runtime_diagnostic *diagnostic) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->line = 0U;
  diagnostic->column = 0U;
  diagnostic->message = NULL;
}

void graphion_runtime_warning_report_init(graphion_runtime_warning_report *report) {
  if (report == NULL) {
    return;
  }
  memset(report, 0, sizeof(*report));
  report->enabled = 1;
}

void graphion_runtime_warning_report_clear(graphion_runtime_warning_report *report) {
  graphion_runtime_warning_report_init(report);
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

static int add_warning(graphion_runtime_warning_report *report,
                       unsigned int line,
                       unsigned int column,
                       const char *message,
                       graphion_runtime_diagnostic *diagnostic) {
  graphion_runtime_warning *warning;
  size_t len;

  if (report == NULL || message == NULL) {
    return fail(diagnostic, line, column, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (!report->enabled) {
    return GINT_OK;
  }
  if (report->count >= GRAPHION_RUNTIME_WARNING_MAX) {
    return fail(diagnostic, line, column, "warning capacity exceeded", GINT_ERR_CAPACITY);
  }
  warning = &report->items[report->count++];
  warning->line = line;
  warning->column = column;
  len = strlen(message);
  if (len >= sizeof(warning->message)) {
    len = sizeof(warning->message) - 1U;
  }
  memcpy(warning->message, message, len);
  warning->message[len] = '\0';
  return GINT_OK;
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

static void runtime_match_case_value_init(runtime_match_case_value *entry) {
  if (entry == NULL) {
    return;
  }
  memset(entry, 0, sizeof(*entry));
  entry->value.kind = GVM_VALUE_NONE;
}

static void runtime_match_case_value_dispose(runtime_match_case_value *entry) {
  if (entry == NULL) {
    return;
  }
  runtime_free_string(&entry->owned_string);
  entry->value.kind = GVM_VALUE_NONE;
  entry->value.as.string_value = NULL;
}

static int runtime_match_case_value_clone(runtime_match_case_value *entry,
                                          const graphion_vm_value *value,
                                          unsigned int line,
                                          graphion_runtime_diagnostic *diagnostic) {
  runtime_match_case_value_init(entry);
  if (entry == NULL || value == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  entry->value = *value;
  if (value->kind == GVM_VALUE_STRING && value->as.string_value != NULL) {
    const size_t len = strlen(value->as.string_value);
    entry->owned_string = (char *)malloc(len + 1U);
    if (entry->owned_string == NULL) {
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    memcpy(entry->owned_string, value->as.string_value, len + 1U);
    entry->value.as.string_value = entry->owned_string;
  }
  return GINT_OK;
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
         strcmp(name, "else") == 0 || strcmp(name, "match") == 0 || strcmp(name, "default") == 0 ||
         strcmp(name, "and") == 0 || strcmp(name, "or") == 0 || strcmp(name, "nand") == 0 ||
         strcmp(name, "nor") == 0 || strcmp(name, "not") == 0;
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

static int program_patch_imm(graphion_runtime_program *program,
                             size_t insn_index,
                             int32_t imm,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic) {
  if (program == NULL || insn_index >= program->program_len) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  program->program[insn_index].imm = imm;
  return GINT_OK;
}

static int program_emit_load_bool(graphion_runtime_program *program,
                                  uint8_t reg,
                                  int bool_value,
                                  unsigned int line,
                                  graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value value;
  size_t const_index;
  int rc;

  vm_value_set_none(&value);
  value.kind = GVM_VALUE_BOOL;
  value.as.bool_value = bool_value != 0 ? 1 : 0;
  rc = program_add_const(program, &value, line, diagnostic, &const_index);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_LOAD_CONST, reg, 0U, (int32_t)const_index, line, diagnostic);
}

static int program_emit_load_int(graphion_runtime_program *program,
                                 uint8_t reg,
                                 int64_t int_value,
                                 unsigned int line,
                                 graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value value;
  size_t const_index;
  int rc;

  vm_value_set_none(&value);
  value.kind = GVM_VALUE_INT;
  value.as.int_value = int_value;
  rc = program_add_const(program, &value, line, diagnostic, &const_index);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_LOAD_CONST, reg, 0U, (int32_t)const_index, line, diagnostic);
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
  if (start[0] == '0' && (start[1] == 'b' || start[1] == 'B')) {
    const char *scan = start + 2;
    uint64_t bits_value = 0U;
    uint8_t width = 0U;

    if (*scan != '0' && *scan != '1') {
      return fail(diagnostic, line, 1U, "expected binary digits after 0b", GINT_ERR_PARSE);
    }
    while (*scan == '0' || *scan == '1') {
      if (width == 64U) {
        return fail(diagnostic, line, 1U, "bits literal too wide", GINT_ERR_PARSE);
      }
      bits_value = (bits_value << 1U) | (uint64_t)(*scan - '0');
      width++;
      scan++;
    }
    if (isdigit((unsigned char)*scan)) {
      return fail(diagnostic, line, 1U, "invalid bits literal", GINT_ERR_PARSE);
    }
    vm_value_set_none(value_out);
    value_out->kind = GVM_VALUE_BITS;
    value_out->reserved[0] = width;
    value_out->as.int_value = (int64_t)bits_value;
    *cursor = scan;
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

static int runtime_value_get_numeric(const graphion_vm_value *value,
                                     int64_t *int_out,
                                     double *float_out,
                                     int *is_float_out) {
  if (value == NULL || int_out == NULL || float_out == NULL || is_float_out == NULL) {
    return 0;
  }
  if (value->kind == GVM_VALUE_INT) {
    *int_out = value->as.int_value;
    *float_out = (double)value->as.int_value;
    *is_float_out = 0;
    return 1;
  }
  if (value->kind == GVM_VALUE_FLOAT) {
    *int_out = (int64_t)value->as.float_value;
    *float_out = value->as.float_value;
    *is_float_out = 1;
    return 1;
  }
  return 0;
}

static int scalar_values_match_equal(const graphion_vm_value *lhs,
                                     const graphion_vm_value *rhs,
                                     int *compatible_out,
                                     int *equal_out) {
  if (compatible_out == NULL || equal_out == NULL || lhs == NULL || rhs == NULL) {
    return 0;
  }
  *compatible_out = 1;
  *equal_out = 0;

  if ((lhs->kind == GVM_VALUE_INT || lhs->kind == GVM_VALUE_FLOAT) &&
      (rhs->kind == GVM_VALUE_INT || rhs->kind == GVM_VALUE_FLOAT)) {
    int64_t lhs_i = 0;
    int64_t rhs_i = 0;
    double lhs_f = 0.0;
    double rhs_f = 0.0;
    int lhs_is_float = 0;
    int rhs_is_float = 0;

    if (!runtime_value_get_numeric(lhs, &lhs_i, &lhs_f, &lhs_is_float) ||
        !runtime_value_get_numeric(rhs, &rhs_i, &rhs_f, &rhs_is_float)) {
      *compatible_out = 0;
      return 1;
    }
    *equal_out = lhs_f == rhs_f;
    return 1;
  }
  if (lhs->kind == GVM_VALUE_BOOL && rhs->kind == GVM_VALUE_INT) {
    if (rhs->as.int_value != 0 && rhs->as.int_value != 1) {
      *compatible_out = 0;
      return 1;
    }
    *equal_out = rhs->as.int_value == (int64_t)lhs->as.bool_value;
    return 1;
  }
  if (lhs->kind == GVM_VALUE_INT && rhs->kind == GVM_VALUE_BOOL) {
    if (lhs->as.int_value != 0 && lhs->as.int_value != 1) {
      *compatible_out = 0;
      return 1;
    }
    *equal_out = lhs->as.int_value == (int64_t)rhs->as.bool_value;
    return 1;
  }
  if (lhs->kind == GVM_VALUE_BOOL && rhs->kind == GVM_VALUE_BOOL) {
    *equal_out = lhs->as.bool_value == rhs->as.bool_value;
    return 1;
  }
  if (lhs->kind == GVM_VALUE_STRING && rhs->kind == GVM_VALUE_STRING) {
    const char *lhs_text = lhs->as.string_value != NULL ? lhs->as.string_value : "";
    const char *rhs_text = rhs->as.string_value != NULL ? rhs->as.string_value : "";
    *equal_out = strcmp(lhs_text, rhs_text) == 0;
    return 1;
  }
  if (lhs->kind == GVM_VALUE_BITS && rhs->kind == GVM_VALUE_BITS) {
    *equal_out = (uint64_t)lhs->as.int_value == (uint64_t)rhs->as.int_value;
    return 1;
  }

  *compatible_out = 0;
  return 1;
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

static int parse_or_expression(const char **cursor,
                               graphion_runtime_program *program,
                               parsed_expr_result *result_out,
                               uint8_t base_reg,
                               unsigned int line,
                               graphion_runtime_diagnostic *diagnostic);

static int parse_comparison_expression(const char **cursor,
                                       graphion_runtime_program *program,
                                       parsed_expr_result *result_out,
                                       uint8_t base_reg,
                                       unsigned int line,
                                       graphion_runtime_diagnostic *diagnostic);

static int parse_bitand_expression(const char **cursor,
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

static int copy_trimmed_segment(const char *start,
                                const char *end,
                                char *buffer,
                                size_t buffer_size,
                                unsigned int line,
                                graphion_runtime_diagnostic *diagnostic) {
  while (start < end && (*start == ' ' || *start == '\t' || *start == '\r')) {
    start++;
  }
  while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) {
    end--;
  }
  if ((size_t)(end - start) >= buffer_size) {
    return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
  }
  memcpy(buffer, start, (size_t)(end - start));
  buffer[end - start] = '\0';
  return GINT_OK;
}

static int scan_ternary_segments(const char *cursor,
                                 const char **true_end_out,
                                 const char **condition_start_out,
                                 const char **condition_end_out,
                                 const char **false_start_out,
                                 const char **expr_end_out) {
  const char *scan = cursor;
  const char *if_pos = NULL;
  const char *else_pos = NULL;
  int depth = 0;
  int in_string = 0;

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
    if (depth == 0) {
      if (if_pos == NULL && strncmp(scan, "if", 2U) == 0 && !is_ident_char(scan[2]) &&
          (scan == cursor || !is_ident_char(scan[-1]))) {
        if_pos = scan;
        scan += 2;
        continue;
      }
      if (if_pos != NULL && else_pos == NULL && strncmp(scan, "else", 4U) == 0 && !is_ident_char(scan[4]) &&
          !is_ident_char(scan[-1])) {
        else_pos = scan;
        scan += 4;
        continue;
      }
    }
    scan++;
  }

  if (if_pos == NULL) {
    return 0;
  }

  if (else_pos == NULL) {
    *true_end_out = if_pos;
    *condition_start_out = if_pos + 2U;
    *condition_end_out = scan;
    *false_start_out = scan;
    *expr_end_out = scan;
    return 2;
  }

  *true_end_out = if_pos;
  *condition_start_out = if_pos + 2U;
  *condition_end_out = else_pos;
  *false_start_out = else_pos + 4U;
  *expr_end_out = scan;
  return 1;
}

static int parse_factor(const char **cursor,
                        graphion_runtime_program *program,
                        parsed_expr_result *result_out,
                        uint8_t base_reg,
                        unsigned int line,
                        graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  int rc;
  skip_spaces(cursor);
  if (**cursor == '-' && !isdigit((unsigned char)(*cursor)[1])) {
    parsed_expr_result rhs;
    const uint8_t target_reg = base_reg;
    const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
    (*cursor)++;
    rc = parse_factor(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit_load_int(program, target_reg, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_SUB, target_reg, scratch_reg, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
  } else if (strncmp(*cursor, "abs", 3U) == 0 && !is_ident_char((*cursor)[3])) {
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

static int parse_comparison_expression(const char **cursor,
                                       graphion_runtime_program *program,
                                       parsed_expr_result *result_out,
                                       uint8_t base_reg,
                                       unsigned int line,
                                       graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  const uint8_t target_reg = base_reg;
  const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
  int rc = parse_bitand_expression(cursor, program, &lhs, base_reg, line, diagnostic);
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
    } else if ((*cursor)[0] == '>' && (*cursor)[1] == '=') {
      cmp_op = GVM_OP_GE;
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
    rc = parse_bitand_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
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

static int parse_bitand_expression(const char **cursor,
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
    skip_spaces(cursor);
    if (**cursor != '&') {
      break;
    }
    (*cursor)++;
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
    rc = program_emit(program, GVM_OP_BIT_AND, target_reg, scratch_reg, 0, line, diagnostic);
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

static int parse_not_expression(const char **cursor,
                                graphion_runtime_program *program,
                                parsed_expr_result *result_out,
                                uint8_t base_reg,
                                unsigned int line,
                                graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result inner;

  skip_spaces(cursor);
  if (strncmp(*cursor, "not", 3U) == 0 && !is_ident_char((*cursor)[3])) {
    int rc;
    *cursor += 3;
    rc = parse_not_expression(cursor, program, &inner, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &inner, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_NOT, base_reg, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    inner.kind = EXPR_RESULT_REG;
    inner.reg_index = base_reg;
    inner.const_index = 0U;
    inner.global_index = 0U;
    *result_out = inner;
    return GINT_OK;
  }

  return parse_comparison_expression(cursor, program, result_out, base_reg, line, diagnostic);
}

static int parse_and_expression(const char **cursor,
                                graphion_runtime_program *program,
                                parsed_expr_result *result_out,
                                uint8_t base_reg,
                                unsigned int line,
                                graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  const uint8_t target_reg = base_reg;
  const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
  int rc = parse_not_expression(cursor, program, &lhs, base_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  for (;;) {
    parsed_expr_result rhs;
    size_t jump_index;
    size_t end_jump_index;
    int short_result;
    skip_spaces(cursor);
    if (strncmp(*cursor, "nand", 4U) == 0 && !is_ident_char((*cursor)[4])) {
      *cursor += 4U;
      rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      jump_index = program->program_len;
      rc = program_emit(program, GVM_OP_JUMP_IF_FALSE, target_reg, 0U, 0, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = parse_not_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = program_emit(program, GVM_OP_NAND, target_reg, scratch_reg, 0, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      end_jump_index = program->program_len;
      rc = program_emit(program, GVM_OP_JUMP, 0U, 0U, 0, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      short_result = 1;
      rc = program_patch_imm(program, jump_index, (int32_t)program->program_len, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = program_emit_load_bool(program, target_reg, short_result, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = program_patch_imm(program, end_jump_index, (int32_t)program->program_len, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      lhs.kind = EXPR_RESULT_REG;
      lhs.reg_index = target_reg;
      lhs.const_index = 0U;
      lhs.global_index = 0U;
      continue;
    }
    if (strncmp(*cursor, "and", 3U) != 0 || is_ident_char((*cursor)[3])) {
      break;
    }
    *cursor += 3U;
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    jump_index = program->program_len;
    rc = program_emit(program, GVM_OP_JUMP_IF_FALSE, target_reg, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = parse_not_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_AND, target_reg, scratch_reg, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    end_jump_index = program->program_len;
    rc = program_emit(program, GVM_OP_JUMP, 0U, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    short_result = 0;
    rc = program_patch_imm(program, jump_index, (int32_t)program->program_len, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit_load_bool(program, target_reg, short_result, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_patch_imm(program, end_jump_index, (int32_t)program->program_len, line, diagnostic);
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

static int parse_or_expression(const char **cursor,
                               graphion_runtime_program *program,
                               parsed_expr_result *result_out,
                               uint8_t base_reg,
                               unsigned int line,
                               graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  const uint8_t target_reg = base_reg;
  const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
  int rc = parse_and_expression(cursor, program, &lhs, base_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  for (;;) {
    parsed_expr_result rhs;
    size_t jump_index;
    size_t end_jump_index;
    int short_result;
    skip_spaces(cursor);
    if (strncmp(*cursor, "nor", 3U) == 0 && !is_ident_char((*cursor)[3])) {
      *cursor += 3U;
      rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      jump_index = program->program_len;
      rc = program_emit(program, GVM_OP_JUMP_IF_TRUE, target_reg, 0U, 0, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = parse_and_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = program_emit(program, GVM_OP_NOR, target_reg, scratch_reg, 0, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      end_jump_index = program->program_len;
      rc = program_emit(program, GVM_OP_JUMP, 0U, 0U, 0, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      short_result = 0;
      rc = program_patch_imm(program, jump_index, (int32_t)program->program_len, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = program_emit_load_bool(program, target_reg, short_result, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = program_patch_imm(program, end_jump_index, (int32_t)program->program_len, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      lhs.kind = EXPR_RESULT_REG;
      lhs.reg_index = target_reg;
      lhs.const_index = 0U;
      lhs.global_index = 0U;
      continue;
    }
    if (strncmp(*cursor, "or", 2U) != 0 || is_ident_char((*cursor)[2])) {
      break;
    }
    *cursor += 2;
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    jump_index = program->program_len;
    rc = program_emit(program, GVM_OP_JUMP_IF_TRUE, target_reg, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = parse_and_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_OR, target_reg, scratch_reg, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    end_jump_index = program->program_len;
    rc = program_emit(program, GVM_OP_JUMP, 0U, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    short_result = 1;
    rc = program_patch_imm(program, jump_index, (int32_t)program->program_len, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit_load_bool(program, target_reg, short_result, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_patch_imm(program, end_jump_index, (int32_t)program->program_len, line, diagnostic);
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
  const char *true_end = NULL;
  const char *condition_start = NULL;
  const char *condition_end = NULL;
  const char *false_start = NULL;
  const char *expr_end = NULL;
  int ternary_scan = scan_ternary_segments(*cursor,
                                           &true_end,
                                           &condition_start,
                                           &condition_end,
                                           &false_start,
                                           &expr_end);

  if (ternary_scan < 0) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (ternary_scan == 1 || ternary_scan == 2) {
    char true_segment[512];
    char condition_segment[512];
    char false_segment[512];
    const char *segment_cursor;
    parsed_expr_result condition_result;
    parsed_expr_result branch_result;
    const uint8_t target_reg = base_reg;
    size_t false_jump_index;
    size_t end_jump_index;
    int rc;

    rc = copy_trimmed_segment(*cursor, true_end, true_segment, sizeof(true_segment), line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    if (true_segment[0] == '\0') {
      return fail(diagnostic, line, 1U, "expected expression before ternary if", GINT_ERR_PARSE);
    }
    rc = copy_trimmed_segment(condition_start, condition_end, condition_segment, sizeof(condition_segment), line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    if (condition_segment[0] == '\0') {
      return fail(diagnostic, line, 1U, "expected condition after ternary if", GINT_ERR_PARSE);
    }
    if (ternary_scan == 2) {
      return fail(diagnostic, line, 1U, "expected else in ternary expression", GINT_ERR_PARSE);
    }
    rc = copy_trimmed_segment(false_start, expr_end, false_segment, sizeof(false_segment), line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    if (false_segment[0] == '\0') {
      return fail(diagnostic, line, 1U, "expected expression after ternary else", GINT_ERR_PARSE);
    }

    segment_cursor = condition_segment;
    rc = parse_or_expression(&segment_cursor, program, &condition_result, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(&segment_cursor);
    if (*segment_cursor != '\0') {
      return fail(diagnostic, line, 1U, "unexpected trailing tokens in ternary condition", GINT_ERR_PARSE);
    }
    rc = ensure_expr_in_reg(program, &condition_result, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    false_jump_index = program->program_len;
    rc = program_emit(program, GVM_OP_JUMP_IF_FALSE, target_reg, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }

    segment_cursor = true_segment;
    rc = parse_expression(&segment_cursor, program, &branch_result, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(&segment_cursor);
    if (*segment_cursor != '\0') {
      return fail(diagnostic, line, 1U, "unexpected trailing tokens in ternary true branch", GINT_ERR_PARSE);
    }
    rc = ensure_expr_in_reg(program, &branch_result, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    end_jump_index = program->program_len;
    rc = program_emit(program, GVM_OP_JUMP, 0U, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }

    rc = program_patch_imm(program, false_jump_index, (int32_t)program->program_len, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    segment_cursor = false_segment;
    rc = parse_expression(&segment_cursor, program, &branch_result, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(&segment_cursor);
    if (*segment_cursor != '\0') {
      return fail(diagnostic, line, 1U, "unexpected trailing tokens in ternary else branch", GINT_ERR_PARSE);
    }
    rc = ensure_expr_in_reg(program, &branch_result, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_patch_imm(program, end_jump_index, (int32_t)program->program_len, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    result_out->kind = EXPR_RESULT_REG;
    result_out->reg_index = target_reg;
    result_out->const_index = 0U;
    result_out->global_index = 0U;
    *cursor = expr_end;
    return GINT_OK;
  }

  return parse_or_expression(cursor, program, result_out, base_reg, line, diagnostic);
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

static int line_is_match_clause(const runtime_source_line *line) {
  return line_starts_with_keyword(line, "match") && !line_keyword_is_assignment_like(line, "match");
}

static int line_is_default_clause(const runtime_source_line *line) {
  return line_starts_with_keyword(line, "default") && !line_keyword_is_assignment_like(line, "default");
}

static size_t find_next_nonblank_line(const runtime_source_line *lines, size_t count, size_t start);

static size_t scan_block_end(const runtime_source_line *lines, size_t count, size_t start, unsigned int block_indent);

static int collect_match_expression_text(const runtime_source_line *lines,
                                         size_t count,
                                         size_t start_index,
                                         char *buffer,
                                         size_t buffer_size,
                                         size_t *header_end_index_out,
                                         unsigned int line,
                                         graphion_runtime_diagnostic *diagnostic);

static int parse_match_case_header(const char *cursor,
                                   graphion_vm_value *value_out,
                                   runtime_match_case_value *owned_value,
                                   unsigned int line,
                                   graphion_runtime_diagnostic *diagnostic);

static const char *scalar_kind_name(const graphion_vm_value *value);

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

static int process_file_level_directives(const char *source,
                                         graphion_runtime_warning_report *report,
                                         graphion_runtime_diagnostic *diagnostic) {
  const char *cursor;
  unsigned int line;

  if (source == NULL || report == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }

  cursor = source;
  line = 1U;
  for (;;) {
    const char *line_start = cursor;
    const char *line_end = cursor;
    const char *trimmed;

    while (*line_end != '\0' && *line_end != '\n') {
      line_end++;
    }
    trimmed = line_start;
    while (trimmed < line_end && (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\r')) {
      trimmed++;
    }

    if (trimmed == line_end) {
      /* blank line before code is allowed */
    } else if (*trimmed == '#') {
      static const char directive_prefix[] = "# graphion:";
      static const char warnings_off_value[] = "warnings=off";
      const size_t directive_prefix_len = sizeof(directive_prefix) - 1U;

      if ((size_t)(line_end - trimmed) >= directive_prefix_len &&
          strncmp(trimmed, directive_prefix, directive_prefix_len) == 0) {
        const char *payload = trimmed + directive_prefix_len;
        const char *payload_end = line_end;
        char directive_value[32];
        size_t payload_len;
        while (payload < payload_end && (*payload == ' ' || *payload == '\t' || *payload == '\r')) {
          payload++;
        }
        while (payload_end > payload && (payload_end[-1] == ' ' || payload_end[-1] == '\t' || payload_end[-1] == '\r')) {
          payload_end--;
        }
        payload_len = (size_t)(payload_end - payload);
        if (payload_len < sizeof(directive_value)) {
          memcpy(directive_value, payload, payload_len);
          directive_value[payload_len] = '\0';
        } else {
          directive_value[0] = '\0';
        }
        if (strcmp(directive_value, warnings_off_value) == 0) {
          report->enabled = 0;
          report->count = 0U;
        } else {
          int rc = add_warning(report, line, 1U, "unknown graphion directive", diagnostic);
          if (rc != GINT_OK) {
            return rc;
          }
        }
      }
    } else {
      break;
    }

    if (*line_end == '\0') {
      break;
    }
    cursor = line_end + 1;
    line++;
  }
  return GINT_OK;
}

static int collect_match_warnings(const runtime_source_line *lines,
                                  size_t count,
                                  graphion_runtime_warning_report *report,
                                  graphion_runtime_diagnostic *diagnostic) {
  size_t i;

  for (i = 0U; i < count; ++i) {
    char match_expression[512];
    size_t header_end_index = i;
    graphion_runtime_program program;
    const char *cursor;
    graphion_vm_value match_literal;
    size_t clause_index;
    unsigned int branch_indent;
    int rc;

    if (!line_is_match_clause(&lines[i])) {
      continue;
    }

    rc = collect_match_expression_text(lines,
                                       count,
                                       i,
                                       match_expression,
                                       sizeof(match_expression),
                                       &header_end_index,
                                       lines[i].line,
                                       diagnostic);
    if (rc != GINT_OK) {
      continue;
    }

    graphion_runtime_program_init(&program);
    cursor = match_expression;
    rc = parse_scalar_literal(&program, &cursor, &match_literal, lines[i].line, diagnostic);
    if (rc != GINT_OK) {
      graphion_runtime_program_dispose(&program);
      i = header_end_index;
      continue;
    }
    skip_spaces(&cursor);
    if (*cursor != '\0') {
      graphion_runtime_program_dispose(&program);
      i = header_end_index;
      continue;
    }

    clause_index = find_next_nonblank_line(lines, count, header_end_index + 1U);
    if (clause_index >= count || lines[clause_index].indent <= lines[i].indent) {
      graphion_runtime_program_dispose(&program);
      i = header_end_index;
      continue;
    }
    branch_indent = lines[clause_index].indent;

    while (clause_index < count) {
      runtime_match_case_value case_value;
      graphion_vm_value parsed_value;
      int compatible = 0;
      int equal = 0;

      if (line_is_blank(&lines[clause_index])) {
        clause_index++;
        continue;
      }
      if (lines[clause_index].indent < branch_indent) {
        break;
      }
      if (lines[clause_index].indent > branch_indent) {
        clause_index++;
        continue;
      }
      if (line_is_default_clause(&lines[clause_index])) {
        clause_index = scan_block_end(lines,
                                      count,
                                      find_next_nonblank_line(lines, count, clause_index + 1U),
                                      find_next_nonblank_line(lines, count, clause_index + 1U) < count
                                          ? lines[find_next_nonblank_line(lines, count, clause_index + 1U)].indent
                                          : branch_indent + 1U);
        continue;
      }

      runtime_match_case_value_init(&case_value);
      rc = parse_match_case_header(line_content(&lines[clause_index]),
                                   &parsed_value,
                                   &case_value,
                                   lines[clause_index].line,
                                   diagnostic);
      if (rc == GINT_OK) {
        scalar_values_match_equal(&match_literal, &parsed_value, &compatible, &equal);
        if (!compatible) {
          char message[GRAPHION_RUNTIME_WARNING_MESSAGE_MAX];
          snprintf(message,
                   sizeof(message),
                   "match case can never match a %s value",
                   scalar_kind_name(&match_literal));
          rc = add_warning(report, lines[clause_index].line, 1U, message, diagnostic);
          runtime_match_case_value_dispose(&case_value);
          if (rc != GINT_OK) {
            graphion_runtime_program_dispose(&program);
            return rc;
          }
        } else {
          runtime_match_case_value_dispose(&case_value);
        }
      }
      clause_index++;
    }

    graphion_runtime_program_dispose(&program);
    i = clause_index > 0U ? clause_index - 1U : i;
  }

  return GINT_OK;
}

int graphion_collect_source_warnings(const char *source,
                                     graphion_runtime_warning_report *report,
                                     graphion_runtime_diagnostic *diagnostic) {
  runtime_source_line lines[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t line_count = 0U;
  int rc;

  if (report == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  graphion_runtime_warning_report_init(report);
  if (diagnostic != NULL) {
    clear_diagnostic(diagnostic);
  }
  if (source == NULL) {
    return fail(diagnostic, 1U, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }

  rc = process_file_level_directives(source, report, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_warning_report_clear(report);
    return rc;
  }
  if (!report->enabled) {
    return GINT_OK;
  }
  rc = split_source_lines(source, lines, GRAPHION_RUNTIME_PROGRAM_MAX, &line_count, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_warning_report_clear(report);
    return rc;
  }
  rc = collect_match_warnings(lines, line_count, report, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_warning_report_clear(report);
    return rc;
  }
  return GINT_OK;
}

void graphion_emit_warning_report(const graphion_runtime_warning_report *report, FILE *stream) {
  size_t i;

  if (report == NULL || stream == NULL || !report->enabled) {
    return;
  }
  for (i = 0U; i < report->count; ++i) {
    fprintf(stream,
            "warning:%u:%u: %s\n",
            report->items[i].line,
            report->items[i].column,
            report->items[i].message);
  }
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

static int condition_line_looks_incomplete(const char *text, size_t length) {
  size_t end = length;

  while (end > 0U && (text[end - 1U] == ' ' || text[end - 1U] == '\t' || text[end - 1U] == '\r')) {
    end--;
  }
  if (end == 0U) {
    return 1;
  }
  {
    const char tail = text[end - 1U];
    if (tail == '(' || tail == '=' || tail == '!' || tail == '<' || tail == '>' || tail == '+' || tail == '-' ||
        tail == '*' || tail == '/' || tail == '%') {
      return 1;
    }
  }
  if (end >= 3U && strncmp(text + end - 3U, "and", 3U) == 0 && (end == 3U || !is_ident_char(text[end - 4U]))) {
    return 1;
  }
  if (end >= 2U && strncmp(text + end - 2U, "or", 2U) == 0 && (end == 2U || !is_ident_char(text[end - 3U]))) {
    return 1;
  }
  if (end >= 4U && strncmp(text + end - 4U, "nand", 4U) == 0 && (end == 4U || !is_ident_char(text[end - 5U]))) {
    return 1;
  }
  if (end >= 3U && strncmp(text + end - 3U, "nor", 3U) == 0 && (end == 3U || !is_ident_char(text[end - 4U]))) {
    return 1;
  }
  if (end >= 3U && strncmp(text + end - 3U, "not", 3U) == 0 && (end == 3U || !is_ident_char(text[end - 4U]))) {
    return 1;
  }
  return 0;
}

static int ternary_line_looks_incomplete(const char *text, size_t length) {
  size_t end = length;

  while (end > 0U && (text[end - 1U] == ' ' || text[end - 1U] == '\t' || text[end - 1U] == '\r')) {
    end--;
  }
  if (end >= 2U && strncmp(text + end - 2U, "if", 2U) == 0 && (end == 2U || !is_ident_char(text[end - 3U]))) {
    return 1;
  }
  if (end >= 4U && strncmp(text + end - 4U, "else", 4U) == 0 && (end == 4U || !is_ident_char(text[end - 5U]))) {
    return 1;
  }
  return 0;
}

static int collect_control_condition_text(const runtime_source_line *lines,
                                          size_t count,
                                          size_t start_index,
                                          const char *keyword,
                                          char *buffer,
                                          size_t buffer_size,
                                          size_t *header_end_index_out,
                                          unsigned int line,
                                          graphion_runtime_diagnostic *diagnostic) {
  const runtime_source_line *start_line;
  const char *cursor;
  size_t keyword_len;
  size_t write_index = 0U;
  size_t i;
  int depth = 0;
  int in_string = 0;
  int multiline_allowed = 0;

  if (lines == NULL || start_index >= count || keyword == NULL || buffer == NULL || buffer_size == 0U ||
      header_end_index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  keyword_len = strlen(keyword);

  start_line = &lines[start_index];
  cursor = line_content(start_line);
  if (strncmp(cursor, keyword, keyword_len) != 0 || is_ident_char(cursor[keyword_len])) {
    return fail(diagnostic, line, 1U, "invalid conditional header", GINT_ERR_PARSE);
  }
  cursor += keyword_len;
  skip_spaces(&cursor);
  if (*cursor == '\0') {
    return fail(diagnostic,
                line,
                1U,
                strcmp(keyword, "if") == 0 ? "expected condition after if" : "expected condition after elif",
                GINT_ERR_PARSE);
  }

  multiline_allowed = *cursor == '(' ? 1 : 0;

  for (i = start_index; i < count; ++i) {
    const char *scan = i == start_index ? cursor : line_content(&lines[i]);

    if (i > start_index) {
      if (!multiline_allowed) {
        return fail(diagnostic, line, 1U, "multiline condition requires grouping parentheses", GINT_ERR_PARSE);
      }
      if (write_index + 1U >= buffer_size) {
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      if (write_index > 0U && buffer[write_index - 1U] != ' ') {
        buffer[write_index++] = ' ';
      }
    }

    while (*scan != '\0') {
      if (in_string) {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        if (*scan == '"') {
          in_string = 0;
        }
        scan++;
        continue;
      }
      if (*scan == '"') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        in_string = 1;
        scan++;
        continue;
      }
      if (*scan == '(') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        depth++;
        scan++;
        continue;
      }
      if (*scan == ')') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        if (depth > 0) {
          depth--;
        }
        scan++;
        continue;
      }
      if (*scan == ':' && depth == 0) {
        const char *tail = scan + 1;
        while (write_index > 0U &&
               (buffer[write_index - 1U] == ' ' || buffer[write_index - 1U] == '\t' || buffer[write_index - 1U] == '\r')) {
          write_index--;
        }
        skip_spaces(&tail);
        if (*tail != '\0') {
          return fail(diagnostic, lines[i].line, 1U, "unexpected trailing tokens after condition", GINT_ERR_PARSE);
        }
        if (write_index == 0U) {
          return fail(diagnostic,
                      line,
                      1U,
                      strcmp(keyword, "if") == 0 ? "expected condition after if" : "expected condition after elif",
                      GINT_ERR_PARSE);
        }
        buffer[write_index] = '\0';
        *header_end_index_out = i;
        return GINT_OK;
      }
      if (write_index + 1U >= buffer_size) {
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      buffer[write_index++] = *scan;
      scan++;
    }

    if (depth == 0) {
      if (i == start_index) {
        if (condition_line_looks_incomplete(buffer, write_index)) {
          return fail(diagnostic, line, 1U, "multiline condition requires grouping parentheses", GINT_ERR_PARSE);
        }
        return fail(diagnostic,
                    line,
                    1U,
                    strcmp(keyword, "if") == 0 ? "expected ':' after if condition" : "expected ':' after elif condition",
                    GINT_ERR_PARSE);
      }
      return fail(diagnostic,
                  lines[i].line,
                  1U,
                  strcmp(keyword, "if") == 0 ? "expected ':' after if condition" : "expected ':' after elif condition",
                  GINT_ERR_PARSE);
    }
  }

  return fail(diagnostic,
              line,
              1U,
              strcmp(keyword, "if") == 0 ? "expected ':' after if condition" : "expected ':' after elif condition",
              GINT_ERR_PARSE);
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

static int collect_match_expression_text(const runtime_source_line *lines,
                                         size_t count,
                                         size_t start_index,
                                         char *buffer,
                                         size_t buffer_size,
                                         size_t *header_end_index_out,
                                         unsigned int line,
                                         graphion_runtime_diagnostic *diagnostic) {
  const runtime_source_line *start_line;
  const char *cursor;
  size_t write_index = 0U;
  size_t i;
  int depth = 0;
  int in_string = 0;
  int multiline_allowed = 0;

  if (lines == NULL || start_index >= count || buffer == NULL || buffer_size == 0U || header_end_index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }

  start_line = &lines[start_index];
  cursor = line_content(start_line);
  if (strncmp(cursor, "match", 5U) != 0 || is_ident_char(cursor[5])) {
    return fail(diagnostic, line, 1U, "invalid match header", GINT_ERR_PARSE);
  }
  cursor += 5;
  skip_spaces(&cursor);
  if (*cursor == '\0') {
    return fail(diagnostic, line, 1U, "expected expression after match", GINT_ERR_PARSE);
  }

  multiline_allowed = *cursor == '(' ? 1 : 0;
  for (i = start_index; i < count; ++i) {
    const char *scan = i == start_index ? cursor : line_content(&lines[i]);

    if (i > start_index) {
      if (!multiline_allowed) {
        return fail(diagnostic, line, 1U, "multiline match expression requires grouping parentheses", GINT_ERR_PARSE);
      }
      if (write_index > 0U && buffer[write_index - 1U] != ' ') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = ' ';
      }
    }

    while (*scan != '\0') {
      if (in_string) {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        if (*scan == '"') {
          in_string = 0;
        }
        scan++;
        continue;
      }
      if (*scan == '"') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        in_string = 1;
        scan++;
        continue;
      }
      if (*scan == '(') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        depth++;
        scan++;
        continue;
      }
      if (*scan == ')') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        if (depth > 0) {
          depth--;
        }
        scan++;
        continue;
      }
      if (*scan == ':' && depth == 0) {
        const char *tail = scan + 1;
        while (write_index > 0U &&
               (buffer[write_index - 1U] == ' ' || buffer[write_index - 1U] == '\t' || buffer[write_index - 1U] == '\r')) {
          write_index--;
        }
        skip_spaces(&tail);
        if (*tail != '\0') {
          return fail(diagnostic, lines[i].line, 1U, "unexpected trailing tokens after match", GINT_ERR_PARSE);
        }
        if (write_index == 0U) {
          return fail(diagnostic, line, 1U, "expected expression after match", GINT_ERR_PARSE);
        }
        buffer[write_index] = '\0';
        *header_end_index_out = i;
        return GINT_OK;
      }
      if (write_index + 1U >= buffer_size) {
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      buffer[write_index++] = *scan;
      scan++;
    }

    if (depth == 0) {
      if (i == start_index) {
        return fail(diagnostic, line, 1U, "expected ':' after match expression", GINT_ERR_PARSE);
      }
      return fail(diagnostic, lines[i].line, 1U, "expected ':' after match expression", GINT_ERR_PARSE);
    }
  }

  return fail(diagnostic, line, 1U, "expected ':' after match expression", GINT_ERR_PARSE);
}

static int parse_match_case_header(const char *cursor,
                                   graphion_vm_value *value_out,
                                   runtime_match_case_value *owned_value,
                                   unsigned int line,
                                   graphion_runtime_diagnostic *diagnostic) {
  graphion_runtime_program program;
  int rc;

  if (cursor == NULL || value_out == NULL || owned_value == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  graphion_runtime_program_init(&program);
  skip_spaces(&cursor);
  rc = parse_scalar_literal(&program, &cursor, value_out, line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != ':') {
    graphion_runtime_program_dispose(&program);
    return fail(diagnostic, line, 1U, "expected ':' after match case", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    graphion_runtime_program_dispose(&program);
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after match case", GINT_ERR_PARSE);
  }
  rc = runtime_match_case_value_clone(owned_value, value_out, line, diagnostic);
  graphion_runtime_program_dispose(&program);
  if (rc != GINT_OK) {
    return rc;
  }
  *value_out = owned_value->value;
  return GINT_OK;
}

static int parse_default_header(const char *cursor,
                                unsigned int line,
                                graphion_runtime_diagnostic *diagnostic) {
  if (strncmp(cursor, "default", 7U) != 0 || is_ident_char(cursor[7])) {
    return fail(diagnostic, line, 1U, "invalid default header", GINT_ERR_PARSE);
  }
  cursor += 7;
  skip_spaces(&cursor);
  if (*cursor != ':') {
    return fail(diagnostic, line, 1U, "expected ':' after default", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after default", GINT_ERR_PARSE);
  }
  return GINT_OK;
}

static const char *scalar_kind_name(const graphion_vm_value *value) {
  if (value == NULL) {
    return "unknown";
  }
  switch (value->kind) {
    case GVM_VALUE_INT:
      return "int";
    case GVM_VALUE_FLOAT:
      return "float";
    case GVM_VALUE_BOOL:
      return "bool";
    case GVM_VALUE_STRING:
      return "string";
    default:
      return "unknown";
  }
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

static int evaluate_expression_text_to_value(const char *expression_text,
                                             size_t expression_len,
                                             graphion_runtime_scope *scope,
                                             unsigned int line,
                                             graphion_runtime_diagnostic *diagnostic,
                                             graphion_vm_value *value_out) {
  char expression_buffer[512];
  const char *cursor = expression_buffer;
  parsed_expr_result expr;
  graphion_runtime_program program;
  int rc;
  if (scope == NULL || expression_text == NULL || value_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (expression_len >= sizeof(expression_buffer)) {
    return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
  }
  memcpy(expression_buffer, expression_text, expression_len);
  expression_buffer[expression_len] = '\0';
  seed_program_from_scope(&program, scope);
  rc = parse_expression(&cursor, &program, &expr, 0U, line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    graphion_runtime_program_dispose(&program);
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after expression", GINT_ERR_PARSE);
  }
  if (expr.kind == EXPR_RESULT_LITERAL) {
    *value_out = program.const_pool[expr.const_index];
  } else if (expr.kind == EXPR_RESULT_GLOBAL) {
    *value_out = scope->globals[expr.global_index];
  } else {
    rc = program_emit(&program, GVM_OP_HALT, 0U, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      graphion_runtime_program_dispose(&program);
      return rc;
    }
    rc = execute_condition_program(&program, scope, expr.reg_index, line, diagnostic, value_out);
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  graphion_runtime_program_dispose(&program);
  return GINT_OK;
}

static int evaluate_condition_text(const char *condition_text,
                                   size_t condition_len,
                                   graphion_runtime_scope *scope,
                                   unsigned int line,
                                   graphion_runtime_diagnostic *diagnostic,
                                   int *result_out) {
  graphion_vm_value value;
  int rc;

  rc = evaluate_expression_text_to_value(condition_text, condition_len, scope, line, diagnostic, &value);
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

static int collect_assignment_statement_text(const runtime_source_line *lines,
                                             size_t count,
                                             size_t start_index,
                                             char *buffer,
                                             size_t buffer_size,
                                             size_t *end_index_out,
                                             unsigned int line,
                                             graphion_runtime_diagnostic *diagnostic) {
  const runtime_source_line *start_line;
  const char *cursor;
  const char *rhs_cursor;
  char target[GRAPHION_RUNTIME_NAME_MAX];
  size_t write_index = 0U;
  size_t i;
  int depth = 0;
  int in_string = 0;
  int multiline_allowed = 0;
  int saw_nonblank_continuation = 0;
  int rc;

  if (lines == NULL || start_index >= count || buffer == NULL || buffer_size == 0U || end_index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }

  start_line = &lines[start_index];
  cursor = line_content(start_line);
  rhs_cursor = cursor;
  rc = parse_identifier_token(&rhs_cursor, target, sizeof(target), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&rhs_cursor);
  if (rhs_cursor[0] == '*' && rhs_cursor[1] == '*' && rhs_cursor[2] == '=') {
    rhs_cursor += 3;
  } else if (rhs_cursor[0] == '/' && rhs_cursor[1] == '/' && rhs_cursor[2] == '=') {
    rhs_cursor += 3;
  } else if ((rhs_cursor[0] == '+' || rhs_cursor[0] == '-' || rhs_cursor[0] == '*' || rhs_cursor[0] == '/' ||
              rhs_cursor[0] == '%') &&
             rhs_cursor[1] == '=') {
    rhs_cursor += 2;
  } else if (*rhs_cursor == '=') {
    rhs_cursor++;
  } else {
    return fail(diagnostic, line, 1U, "expected '='", GINT_ERR_PARSE);
  }
  skip_spaces(&rhs_cursor);
  multiline_allowed = *rhs_cursor == '(' ? 1 : 0;

  for (i = start_index; i < count; ++i) {
    const char *scan = i == start_index ? cursor : line_content(&lines[i]);

    if (i > start_index) {
      if (line_is_blank(&lines[i])) {
        continue;
      }
      saw_nonblank_continuation = 1;
      if (!multiline_allowed) {
        return fail(diagnostic, line, 1U, "multiline assignment expression requires grouping parentheses", GINT_ERR_PARSE);
      }
      if (write_index > 0U && buffer[write_index - 1U] != ' ') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = ' ';
      }
    }

    while (*scan != '\0') {
      if (in_string) {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        if (*scan == '"') {
          in_string = 0;
        }
        scan++;
        continue;
      }
      if (*scan == '"') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        in_string = 1;
        scan++;
        continue;
      }
      if (*scan == '(') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        depth++;
        scan++;
        continue;
      }
      if (*scan == ')') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        if (depth > 0) {
          depth--;
        }
        scan++;
        continue;
      }
      if (write_index + 1U >= buffer_size) {
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      buffer[write_index++] = *scan;
      scan++;
    }

    while (write_index > 0U &&
           (buffer[write_index - 1U] == ' ' || buffer[write_index - 1U] == '\t' || buffer[write_index - 1U] == '\r')) {
      write_index--;
    }
    if (depth == 0) {
      if (i == start_index && ternary_line_looks_incomplete(buffer, write_index) &&
          find_next_nonblank_line(lines, count, i + 1U) < count) {
        return fail(diagnostic, line, 1U, "multiline assignment expression requires grouping parentheses", GINT_ERR_PARSE);
      }
      buffer[write_index] = '\0';
      *end_index_out = i;
      return GINT_OK;
    }
  }

  if (!saw_nonblank_continuation) {
    buffer[write_index] = '\0';
    *end_index_out = start_index;
    return GINT_OK;
  }
  return fail(diagnostic, line, 1U, "expected ')' after expression", GINT_ERR_PARSE);
}

static int execute_statement_source_line(const runtime_source_line *lines,
                                         size_t count,
                                         size_t *index,
                                         graphion_runtime_scope *scope,
                                         graphion_runtime_diagnostic *diagnostic,
                                         FILE *output) {
  char statement_text[512];
  const char *statement_source = line_content(&lines[*index]);
  graphion_runtime_program program;
  size_t statement_end = *index;
  int rc;

  if (!(strncmp(statement_source, "print", 5U) == 0 && !is_ident_char(statement_source[5]))) {
    rc = collect_assignment_statement_text(lines,
                                           count,
                                           *index,
                                           statement_text,
                                           sizeof(statement_text),
                                           &statement_end,
                                           lines[*index].line,
                                           diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    statement_source = statement_text;
  }
  seed_program_from_scope(&program, scope);
  rc = parse_statement_line(statement_source, scope, &program, lines[*index].line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  if (program.program_len > 0U) {
    rc = graphion_execute_program(&program, scope, diagnostic, output);
    graphion_runtime_program_dispose(&program);
    if (rc != GINT_OK) {
      return rc;
    }
  } else {
    graphion_runtime_program_dispose(&program);
  }
  *index = statement_end + 1U;
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
    if (is_else_clause) {
      int rc = parse_else_header(cursor, clause_line->line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      body_start = find_next_nonblank_line(lines, count, clause_index + 1U);
      if (body_start >= count || lines[body_start].indent <= current_indent) {
        return fail(diagnostic, clause_line->line, 1U, "expected indented block after else", GINT_ERR_PARSE);
      }
      body_indent = lines[body_start].indent;
      body_end = scan_block_end(lines, count, body_start, body_indent);
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
      char condition_text[512];
      size_t header_end_index = clause_index;
      int rc = collect_control_condition_text(lines,
                                              count,
                                              clause_index,
                                              line_is_elif_clause(clause_line) ? "elif" : "if",
                                              condition_text,
                                              sizeof(condition_text),
                                              &header_end_index,
                                              clause_line->line,
                                              diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      body_start = find_next_nonblank_line(lines, count, header_end_index + 1U);
      if (body_start >= count || lines[body_start].indent <= current_indent) {
        return fail(diagnostic,
                    clause_line->line,
                    1U,
                    line_is_elif_clause(clause_line) ? "expected indented block after elif" :
                    "expected indented block after if",
                    GINT_ERR_PARSE);
      }
      body_indent = lines[body_start].indent;
      body_end = scan_block_end(lines, count, body_start, body_indent);
      if (!branch_taken) {
        int condition_true = 0;
        rc = evaluate_condition_text(condition_text,
                                     strlen(condition_text),
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

static int execute_match_statement(const runtime_source_line *lines,
                                   size_t count,
                                   size_t *index,
                                   unsigned int current_indent,
                                   graphion_runtime_scope *scope,
                                   graphion_runtime_diagnostic *diagnostic,
                                   FILE *output) {
  char match_expression[512];
  graphion_vm_value match_value;
  runtime_match_case_value seen_cases[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t seen_case_count = 0U;
  size_t header_end_index = *index;
  size_t clause_index;
  unsigned int branch_indent;
  int branch_taken = 0;
  int seen_default = 0;
  int rc;
  size_t i;

  for (i = 0U; i < GRAPHION_RUNTIME_WARNING_MAX; ++i) {
    runtime_match_case_value_init(&seen_cases[i]);
  }

  rc = collect_match_expression_text(lines,
                                     count,
                                     *index,
                                     match_expression,
                                     sizeof(match_expression),
                                     &header_end_index,
                                     lines[*index].line,
                                     diagnostic);
  if (rc != GINT_OK) {
    goto cleanup;
  }

  rc = evaluate_expression_text_to_value(match_expression,
                                         strlen(match_expression),
                                         scope,
                                         lines[*index].line,
                                         diagnostic,
                                         &match_value);
  if (rc != GINT_OK) {
    goto cleanup;
  }

  clause_index = find_next_nonblank_line(lines, count, header_end_index + 1U);
  if (clause_index >= count || lines[clause_index].indent <= current_indent) {
    rc = fail(diagnostic, lines[*index].line, 1U, "expected indented match block", GINT_ERR_PARSE);
    goto cleanup;
  }
  branch_indent = lines[clause_index].indent;

  while (clause_index < count) {
    size_t label_start = clause_index;
    size_t label_index = clause_index;
    size_t body_start;
    size_t body_end;
    int label_matches = 0;
    int is_default = 0;

    if (line_is_blank(&lines[clause_index])) {
      clause_index++;
      continue;
    }
    if (lines[clause_index].indent < branch_indent) {
      break;
    }
    if (lines[clause_index].indent > branch_indent) {
      rc = fail(diagnostic, lines[clause_index].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
      goto cleanup;
    }

    if (line_is_default_clause(&lines[clause_index])) {
      is_default = 1;
      if (seen_default) {
        rc = fail(diagnostic, lines[clause_index].line, 1U, "default can only appear once", GINT_ERR_PARSE);
        goto cleanup;
      }
      rc = parse_default_header(line_content(&lines[clause_index]), lines[clause_index].line, diagnostic);
      if (rc != GINT_OK) {
        goto cleanup;
      }
      seen_default = 1;
      label_index = clause_index + 1U;
    } else {
      while (label_index < count) {
        runtime_match_case_value case_value;
        graphion_vm_value parsed_value;
        int compatible = 0;
        int equal = 0;
        size_t existing_index;

        runtime_match_case_value_init(&case_value);
        if (line_is_blank(&lines[label_index])) {
          label_index++;
          continue;
        }
        if (lines[label_index].indent != branch_indent || line_is_default_clause(&lines[label_index])) {
          break;
        }
        rc = parse_match_case_header(line_content(&lines[label_index]),
                                     &parsed_value,
                                     &case_value,
                                     lines[label_index].line,
                                     diagnostic);
        if (rc != GINT_OK) {
          runtime_match_case_value_dispose(&case_value);
          goto cleanup;
        }
        for (existing_index = 0U; existing_index < seen_case_count; ++existing_index) {
          scalar_values_match_equal(&seen_cases[existing_index].value, &parsed_value, &compatible, &equal);
          if (compatible && equal) {
            runtime_match_case_value_dispose(&case_value);
            rc = fail(diagnostic, lines[label_index].line, 1U, "duplicate match case", GINT_ERR_PARSE);
            goto cleanup;
          }
        }
        if (seen_case_count >= GRAPHION_RUNTIME_PROGRAM_MAX) {
          runtime_match_case_value_dispose(&case_value);
          rc = fail(diagnostic, lines[label_index].line, 1U, "too many match cases", GINT_ERR_CAPACITY);
          goto cleanup;
        }
        seen_cases[seen_case_count++] = case_value;
        scalar_values_match_equal(&match_value, &parsed_value, &compatible, &equal);
        if (compatible && equal) {
          label_matches = 1;
        }
        label_index++;
        if (label_index >= count || line_is_blank(&lines[label_index]) || lines[label_index].indent != branch_indent) {
          break;
        }
      }
      if (label_index == label_start) {
        rc = fail(diagnostic, lines[clause_index].line, 1U, "expected scalar literal", GINT_ERR_PARSE);
        goto cleanup;
      }
    }

    body_start = find_next_nonblank_line(lines, count, label_index);
    if (body_start >= count || lines[body_start].indent <= branch_indent) {
      rc = fail(diagnostic,
                lines[label_start].line,
                1U,
                is_default ? "expected indented block after default" : "expected indented block after match case",
                GINT_ERR_PARSE);
      goto cleanup;
    }
    body_end = scan_block_end(lines, count, body_start, lines[body_start].indent);

    if (is_default && find_next_nonblank_line(lines, count, body_end) < count &&
        lines[find_next_nonblank_line(lines, count, body_end)].indent == branch_indent) {
      rc = fail(diagnostic, lines[label_start].line, 1U, "default must be last in match", GINT_ERR_PARSE);
      goto cleanup;
    }

    if (!branch_taken && (is_default || label_matches)) {
      size_t exec_index = body_start;
      rc = execute_block(lines, count, &exec_index, lines[body_start].indent, scope, diagnostic, output);
      if (rc != GINT_OK) {
        goto cleanup;
      }
      body_end = exec_index;
      branch_taken = 1;
    }

    clause_index = body_end;
  }

  *index = clause_index;
  rc = GINT_OK;

cleanup:
  for (i = 0U; i < seen_case_count; ++i) {
    runtime_match_case_value_dispose(&seen_cases[i]);
  }
  return rc;
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
    if (line_is_default_clause(&lines[i])) {
      return fail(diagnostic, lines[i].line, 1U, "default without matching match", GINT_ERR_PARSE);
    }
    if (line_is_if_clause(&lines[i])) {
      int rc = execute_if_chain(lines, count, &i, block_indent, scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
      continue;
    }
    if (line_is_match_clause(&lines[i])) {
      int rc = execute_match_statement(lines, count, &i, block_indent, scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
      continue;
    }
    {
      int rc = execute_statement_source_line(lines, count, &i, scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
    }
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

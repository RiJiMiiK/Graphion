/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/operands.h"

#include <math.h>
int parse_identifier_token(const char **cursor,
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

int parse_string_literal(graphion_runtime_program *program,
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

int parse_scalar_literal(graphion_runtime_program *program,
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
  if (strncmp(start, "pi", 2U) == 0 && !is_ident_char(start[2])) {
    value_out->kind = GVM_VALUE_FLOAT;
    value_out->as.float_value = 3.14159265358979323846;
    *cursor += 2;
    return GINT_OK;
  }
  if (strncmp(start, "tau", 3U) == 0 && !is_ident_char(start[3])) {
    value_out->kind = GVM_VALUE_FLOAT;
    value_out->as.float_value = 6.28318530717958647692;
    *cursor += 3;
    return GINT_OK;
  }
  if (start[0] == 'e' && !is_ident_char(start[1])) {
    value_out->kind = GVM_VALUE_FLOAT;
    value_out->as.float_value = 2.71828182845904523536;
    *cursor += 1;
    return GINT_OK;
  }
  if (strncmp(start, "nan", 3U) == 0 && !is_ident_char(start[3])) {
    value_out->kind = GVM_VALUE_FLOAT;
    value_out->as.float_value = NAN;
    *cursor += 3;
    return GINT_OK;
  }
  if (strncmp(start, "inf", 3U) == 0 && !is_ident_char(start[3])) {
    value_out->kind = GVM_VALUE_FLOAT;
    value_out->as.float_value = INFINITY;
    *cursor += 3;
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

int parse_operand(const char **cursor,
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
    if (strcmp(name, "true") == 0 || strcmp(name, "false") == 0 || strcmp(name, "pi") == 0 ||
        strcmp(name, "tau") == 0 || strcmp(name, "e") == 0 || strcmp(name, "nan") == 0 ||
        strcmp(name, "inf") == 0) {
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

int emit_load_operand(graphion_runtime_program *program,
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

int ensure_expr_in_reg(graphion_runtime_program *program,
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


int copy_trimmed_segment(const char *start,
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

int scan_ternary_segments(const char *cursor,
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


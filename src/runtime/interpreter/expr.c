/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/expr_internal.h"

int parse_expression(const char **cursor,
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

static unsigned int expression_column(const char *segment_start, const char *cursor) {
  if (segment_start == NULL || cursor == NULL || cursor < segment_start) {
    return 1U;
  }
  return (unsigned int)(cursor - segment_start) + 1U;
}

static const char *expression_operator_at(const char *cursor) {
  skip_spaces(&cursor);
  if (cursor[0] == '*' && cursor[1] == '*') {
    return "**";
  }
  if (cursor[0] == '/' && cursor[1] == '/') {
    return "//";
  }
  if (cursor[0] == '<' && cursor[1] == '<') {
    return "<<";
  }
  if (cursor[0] == '>' && cursor[1] == '>') {
    return ">>";
  }
  if (cursor[0] == '=' && cursor[1] == '=') {
    return "==";
  }
  if (cursor[0] == '!' && cursor[1] == '=') {
    return "!=";
  }
  if (cursor[0] == '<' && cursor[1] == '=') {
    return "<=";
  }
  if (cursor[0] == '>' && cursor[1] == '=') {
    return ">=";
  }
  if (strncmp(cursor, "and", 3U) == 0 && !is_ident_char(cursor[3])) {
    return "and";
  }
  if (strncmp(cursor, "or", 2U) == 0 && !is_ident_char(cursor[2])) {
    return "or";
  }
  if (strncmp(cursor, "nand", 4U) == 0 && !is_ident_char(cursor[4])) {
    return "nand";
  }
  if (strncmp(cursor, "nor", 3U) == 0 && !is_ident_char(cursor[3])) {
    return "nor";
  }
  switch (cursor[0]) {
    case '+':
      return "+";
    case '-':
      return isdigit((unsigned char)cursor[1]) ? NULL : "-";
    case '*':
      return "*";
    case '/':
      return "/";
    case '%':
      return "%";
    case '&':
      return "&";
    case '|':
      return "|";
    case '^':
      return "^";
    case '!':
      return "!";
    case '<':
      return "<";
    case '>':
      return ">";
    default:
      break;
  }
  return NULL;
}

static int fail_expected_expression_near_operator(
    graphion_runtime_diagnostic *diagnostic,
    unsigned int line,
    const char *position,
    const char *op) {
  char message[128];

  snprintf(message, sizeof(message), "expected expression %s '%s'", position, op);
  return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
}

static int remap_missing_rhs_operator_error(int rc,
                                            graphion_runtime_diagnostic *diagnostic,
                                            unsigned int line,
                                            const char *op) {
  if (rc == GINT_ERR_PARSE && diagnostic != NULL &&
      diagnostic->message != NULL &&
      (strcmp(diagnostic->message, "expected scalar literal") == 0 ||
       strncmp(diagnostic->message, "expected expression before ", 27U) == 0)) {
    return fail_expected_expression_near_operator(diagnostic, line, "after", op);
  }
  return rc;
}

static int parse_bitor_expression(const char **cursor,
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

static int parse_shift_expression(const char **cursor,
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

static int parenthesized_form_is_tuple(const char *cursor, int *is_tuple_out, int *is_empty_out) {
  const char *scan;
  int depth = 0;
  int in_string = 0;

  if (cursor == NULL || is_tuple_out == NULL || is_empty_out == NULL || *cursor != '(') {
    return 0;
  }
  *is_tuple_out = 0;
  *is_empty_out = 0;

  scan = cursor + 1;
  skip_spaces(&scan);
  if (*scan == ')') {
    *is_empty_out = 1;
    return 1;
  }

  scan = cursor + 1;
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
        return 1;
      }
      depth--;
      scan++;
      continue;
    }
    if (*scan == ',' && depth == 0) {
      *is_tuple_out = 1;
      return 1;
    }
    scan++;
  }
  return 1;
}

static int try_parse_struct_instance_literal(const char **cursor,
                                             graphion_runtime_program *program,
                                             parsed_expr_result *result_out,
                                             uint8_t base_reg,
                                             unsigned int line,
                                             graphion_runtime_diagnostic *diagnostic) {
  const char *saved;
  const char *scan;
  char type_name[GRAPHION_RUNTIME_NAME_MAX];
  int global_index;
  parsed_expr_result fields_expr;
  int rc;

  if (cursor == NULL || *cursor == NULL || program == NULL || result_out == NULL ||
      !is_ident_start_char(**cursor)) {
    return 0;
  }
  saved = *cursor;
  scan = saved;
  rc = parse_identifier_token(&scan, type_name, sizeof(type_name), line, diagnostic);
  if (rc != GINT_OK) {
    *cursor = saved;
    return 0;
  }
  skip_spaces(&scan);
  if (*scan != '{') {
    *cursor = saved;
    return 0;
  }
  global_index = program_find_global_index(program, type_name);
  if (global_index < 0) {
    *cursor = saved;
    return fail(diagnostic, line, expression_column(saved, saved), "unknown struct type", GINT_ERR_UNKNOWN_VARIABLE);
  }
  *cursor = scan;
  rc = program_emit(program, GVM_OP_LOAD_GLOBAL, base_reg, 0U, global_index, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = parse_dict_literal(cursor, program, &fields_expr, (uint8_t)(base_reg + 1U), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = ensure_expr_in_reg(program, &fields_expr, (uint8_t)(base_reg + 1U), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, GVM_OP_STRUCT_NEW, base_reg, (uint8_t)(base_reg + 1U), 0, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  result_out->kind = EXPR_RESULT_REG;
  result_out->reg_index = base_reg;
  result_out->const_index = 0U;
  result_out->global_index = 0U;
  return 1;
}

static int parse_primary_expression(const char **cursor,
                                    graphion_runtime_program *program,
                                    parsed_expr_result *result_out,
                                    uint8_t base_reg,
                                    unsigned int line,
                                    graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  int is_tuple_form = 0;
  int is_empty_tuple = 0;
  int rc;

  skip_spaces(cursor);
  if ((rc = try_parse_struct_instance_literal(cursor, program, &lhs, base_reg, line, diagnostic)) != 0) {
    if (rc < 0) {
      return rc;
    }
  } else if (strncmp(*cursor, "set", 3U) == 0 && !is_ident_char((*cursor)[3])) {
    rc = parse_set_literal(cursor, program, &lhs, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  } else if ((rc = try_parse_direct_builtin(cursor, program, &lhs, base_reg, line, diagnostic)) != 0) {
    if (rc < 0) {
      return rc;
    }
  } else if ((rc = try_parse_special_builtin(cursor, program, &lhs, base_reg, line, diagnostic)) != 0) {
    if (rc < 0) {
      return rc;
    }
  } else if ((rc = try_parse_opcode_builtin(cursor, program, &lhs, base_reg, line, diagnostic)) != 0) {
    if (rc < 0) {
      return rc;
    }
  } else if (**cursor == '(') {
    if (!parenthesized_form_is_tuple(*cursor, &is_tuple_form, &is_empty_tuple)) {
      return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
    }
    if (is_empty_tuple) {
      return fail(diagnostic, line, 1U, "empty tuple literal is not supported", GINT_ERR_PARSE);
    }
    if (is_tuple_form) {
      rc = parse_tuple_literal(cursor, program, &lhs, base_reg, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
    } else {
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
    }
  } else if (**cursor == '[') {
    rc = parse_list_literal(cursor, program, &lhs, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  } else if (**cursor == '{') {
    rc = parse_dict_literal(cursor, program, &lhs, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  } else {
    parsed_operand operand;
    const char *op = expression_operator_at(*cursor);

    if (op != NULL) {
      return fail_expected_expression_near_operator(diagnostic, line, "before", op);
    }
    rc = parse_operand(cursor, program, &operand, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = operand.kind == OPERAND_LITERAL ? EXPR_RESULT_LITERAL : EXPR_RESULT_GLOBAL;
    lhs.const_index = operand.const_index;
    lhs.global_index = operand.global_index;
    lhs.reg_index = 0U;
  }

  for (;;) {
    parsed_expr_result index_expr;
    const uint8_t target_reg = base_reg;
    const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);

    skip_spaces(cursor);
    if (**cursor != '[') {
      break;
    }
    (*cursor)++;
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = parse_expression(cursor, program, &index_expr, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &index_expr, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ']') {
      return fail(diagnostic, line, 1U, "expected ']' after index expression", GINT_ERR_PARSE);
    }
    (*cursor)++;
    rc = program_emit(program, GVM_OP_LIST_GET, target_reg, scratch_reg, 0, line, diagnostic);
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

static int parse_factor(const char **cursor,
                        graphion_runtime_program *program,
                        parsed_expr_result *result_out,
                        uint8_t base_reg,
                        unsigned int line,
                        graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  int rc;
  skip_spaces(cursor);
  if (**cursor == '~') {
    const uint8_t target_reg = base_reg;
    (*cursor)++;
    rc = parse_factor(cursor, program, &lhs, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_BIT_NOT, target_reg, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
  } else if (**cursor == '-' && !isdigit((unsigned char)(*cursor)[1])) {
    parsed_expr_result rhs;
    const uint8_t target_reg = base_reg;
    const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
    (*cursor)++;
    rc = parse_factor(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return remap_missing_rhs_operator_error(rc, diagnostic, line, "-");
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
  } else {
    rc = parse_primary_expression(cursor, program, &lhs, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  skip_spaces(cursor);
  if ((*cursor)[0] == '*' && (*cursor)[1] == '*') {
    parsed_expr_result rhs;
    const uint8_t target_reg = base_reg;
    const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
    *cursor += 2;
    rc = parse_factor(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return remap_missing_rhs_operator_error(rc, diagnostic, line, "**");
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
  skip_spaces(cursor);
  while (**cursor == '!' && (*cursor)[1] != '=') {
    const uint8_t target_reg = base_reg;
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_FACTORIAL, target_reg, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
    (*cursor)++;
    skip_spaces(cursor);
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
      return remap_missing_rhs_operator_error(
          rc, diagnostic, line, floor_div ? "//" : op == '*' ? "*" : op == '/' ? "/" : "%");
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
      return remap_missing_rhs_operator_error(rc, diagnostic, line, op == '+' ? "+" : "-");
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
  int rc = parse_bitor_expression(cursor, program, &lhs, base_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  for (;;) {
    parsed_expr_result rhs;
    graphion_opcode cmp_op;
    const char *cmp_op_text;
    skip_spaces(cursor);
    if ((*cursor)[0] == '=' && (*cursor)[1] == '=') {
      cmp_op = GVM_OP_EQ;
      cmp_op_text = "==";
      *cursor += 2;
    } else if ((*cursor)[0] == '!' && (*cursor)[1] == '=') {
      cmp_op = GVM_OP_NE;
      cmp_op_text = "!=";
      *cursor += 2;
    } else if ((*cursor)[0] == '<' && (*cursor)[1] == '=') {
      cmp_op = GVM_OP_LE;
      cmp_op_text = "<=";
      *cursor += 2;
    } else if ((*cursor)[0] == '>' && (*cursor)[1] == '=') {
      cmp_op = GVM_OP_GE;
      cmp_op_text = ">=";
      *cursor += 2;
    } else if ((*cursor)[0] == '<' && (*cursor)[1] != '=') {
      cmp_op = GVM_OP_LT;
      cmp_op_text = "<";
      *cursor += 1;
    } else if ((*cursor)[0] == '>' && (*cursor)[1] != '=') {
      cmp_op = GVM_OP_GT;
      cmp_op_text = ">";
      *cursor += 1;
    } else {
      break;
    }
    rc = parse_bitor_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return remap_missing_rhs_operator_error(rc, diagnostic, line, cmp_op_text);
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

static int parse_bitor_expression(const char **cursor,
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
    skip_spaces(cursor);
    if (**cursor != '|' && **cursor != '^') {
      break;
    }
    {
      const graphion_opcode bit_op = **cursor == '|' ? GVM_OP_BIT_OR : GVM_OP_BIT_XOR;
      (*cursor)++;
      rc = parse_bitand_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
      if (rc != GINT_OK) {
        return remap_missing_rhs_operator_error(
            rc, diagnostic, line, bit_op == GVM_OP_BIT_OR ? "|" : "^");
      }
      rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      rc = program_emit(program, bit_op, target_reg, scratch_reg, 0, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
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
  int rc = parse_shift_expression(cursor, program, &lhs, base_reg, line, diagnostic);
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
    rc = parse_shift_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return remap_missing_rhs_operator_error(rc, diagnostic, line, "&");
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

static int parse_shift_expression(const char **cursor,
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
    graphion_opcode shift_op;
    if ((*cursor)[0] == '<' && (*cursor)[1] == '<') {
      shift_op = GVM_OP_BIT_SHL;
    } else if ((*cursor)[0] == '>' && (*cursor)[1] == '>') {
      shift_op = GVM_OP_BIT_SHR;
    } else {
      break;
    }
    *cursor += 2;
    rc = parse_additive_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return remap_missing_rhs_operator_error(
          rc, diagnostic, line, shift_op == GVM_OP_BIT_SHL ? "<<" : ">>");
    }
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, shift_op, target_reg, scratch_reg, 0, line, diagnostic);
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
      return remap_missing_rhs_operator_error(rc, diagnostic, line, "not");
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
        return remap_missing_rhs_operator_error(rc, diagnostic, line, "nand");
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
      return remap_missing_rhs_operator_error(rc, diagnostic, line, "and");
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
        return remap_missing_rhs_operator_error(rc, diagnostic, line, "nor");
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
      return remap_missing_rhs_operator_error(rc, diagnostic, line, "or");
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

int parse_expression(const char **cursor,
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
    const char *expression_start = *cursor;
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
      return fail(diagnostic,
                  line,
                  expression_column(expression_start, expression_start),
                  "expected expression before ternary if",
                  GINT_ERR_PARSE);
    }
    rc = copy_trimmed_segment(condition_start, condition_end, condition_segment, sizeof(condition_segment), line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    if (condition_segment[0] == '\0') {
      const char *missing_condition = condition_start;
      skip_spaces(&missing_condition);
      return fail(diagnostic,
                  line,
                  expression_column(expression_start, missing_condition),
                  "expected condition after ternary if",
                  GINT_ERR_PARSE);
    }
    if (ternary_scan == 2) {
      return fail(diagnostic,
                  line,
                  expression_column(expression_start, condition_end),
                  "expected else in ternary expression",
                  GINT_ERR_PARSE);
    }
    rc = copy_trimmed_segment(false_start, expr_end, false_segment, sizeof(false_segment), line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    if (false_segment[0] == '\0') {
      const char *missing_false_branch = false_start;
      skip_spaces(&missing_false_branch);
      return fail(diagnostic,
                  line,
                  expression_column(expression_start, missing_false_branch),
                  "expected expression after ternary else",
                  GINT_ERR_PARSE);
    }

    segment_cursor = condition_segment;
    rc = parse_or_expression(&segment_cursor, program, &condition_result, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(&segment_cursor);
    if (*segment_cursor != '\0') {
      return fail(diagnostic,
                  line,
                  expression_column(expression_start, condition_start) +
                      (unsigned int)(segment_cursor - condition_segment),
                  "unexpected trailing tokens in ternary condition",
                  GINT_ERR_PARSE);
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
      return fail(diagnostic,
                  line,
                  expression_column(expression_start, expression_start) +
                      (unsigned int)(segment_cursor - true_segment),
                  "unexpected trailing tokens in ternary true branch",
                  GINT_ERR_PARSE);
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
      return fail(diagnostic,
                  line,
                  expression_column(expression_start, false_start) +
                      (unsigned int)(segment_cursor - false_segment),
                  "unexpected trailing tokens in ternary else branch",
                  GINT_ERR_PARSE);
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

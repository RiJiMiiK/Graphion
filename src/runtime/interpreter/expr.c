/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/expr.h"

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
  } else if (strncmp(*cursor, "min", 3U) == 0 && !is_ident_char((*cursor)[3])) {
    parsed_expr_result rhs;
    const uint8_t target_reg = base_reg;
    const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
    const char *after_name = *cursor + 3;
    skip_spaces(&after_name);
    if (*after_name != '(') {
      return fail(diagnostic, line, 1U, "expected '(' after min", GINT_ERR_PARSE);
    }
    *cursor = after_name + 1;
    rc = parse_expression(cursor, program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ',') {
      return fail(diagnostic, line, 1U, "expected ',' between min arguments", GINT_ERR_PARSE);
    }
    (*cursor)++;
    rc = parse_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ')') {
      return fail(diagnostic, line, 1U, "expected ')' after min arguments", GINT_ERR_PARSE);
    }
    (*cursor)++;
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_MIN, target_reg, scratch_reg, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
  } else if (strncmp(*cursor, "max", 3U) == 0 && !is_ident_char((*cursor)[3])) {
    parsed_expr_result rhs;
    const uint8_t target_reg = base_reg;
    const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
    const char *after_name = *cursor + 3;
    skip_spaces(&after_name);
    if (*after_name != '(') {
      return fail(diagnostic, line, 1U, "expected '(' after max", GINT_ERR_PARSE);
    }
    *cursor = after_name + 1;
    rc = parse_expression(cursor, program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ',') {
      return fail(diagnostic, line, 1U, "expected ',' between max arguments", GINT_ERR_PARSE);
    }
    (*cursor)++;
    rc = parse_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ')') {
      return fail(diagnostic, line, 1U, "expected ')' after max arguments", GINT_ERR_PARSE);
    }
    (*cursor)++;
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_MAX, target_reg, scratch_reg, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
  } else if (strncmp(*cursor, "clamp", 5U) == 0 && !is_ident_char((*cursor)[5])) {
    parsed_expr_result lo;
    parsed_expr_result hi;
    const uint8_t target_reg = base_reg;
    const uint8_t lo_reg = (uint8_t)(base_reg + 1U);
    const uint8_t hi_reg = (uint8_t)(base_reg + 2U);
    const char *after_name = *cursor + 5;
    skip_spaces(&after_name);
    if (*after_name != '(') {
      return fail(diagnostic, line, 1U, "expected '(' after clamp", GINT_ERR_PARSE);
    }
    *cursor = after_name + 1;
    rc = parse_expression(cursor, program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ',') {
      return fail(diagnostic, line, 1U, "expected ',' after clamp value", GINT_ERR_PARSE);
    }
    (*cursor)++;
    rc = parse_expression(cursor, program, &lo, lo_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ',') {
      return fail(diagnostic, line, 1U, "expected ',' after clamp lower bound", GINT_ERR_PARSE);
    }
    (*cursor)++;
    rc = parse_expression(cursor, program, &hi, hi_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ')') {
      return fail(diagnostic, line, 1U, "expected ')' after clamp arguments", GINT_ERR_PARSE);
    }
    (*cursor)++;
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &lo, lo_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &hi, hi_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_CLAMP, target_reg, lo_reg, hi_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    lhs.kind = EXPR_RESULT_REG;
    lhs.reg_index = target_reg;
    lhs.const_index = 0U;
    lhs.global_index = 0U;
  } else if (strncmp(*cursor, "sqrt", 4U) == 0 && !is_ident_char((*cursor)[4])) {
    const uint8_t target_reg = base_reg;
    const char *after_name = *cursor + 4;
    skip_spaces(&after_name);
    if (*after_name != '(') {
      return fail(diagnostic, line, 1U, "expected '(' after sqrt", GINT_ERR_PARSE);
    }
    *cursor = after_name + 1;
    rc = parse_expression(cursor, program, &lhs, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ')') {
      return fail(diagnostic, line, 1U, "expected ')' after sqrt argument", GINT_ERR_PARSE);
    }
    (*cursor)++;
    rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_SQRT, target_reg, 0U, 0, line, diagnostic);
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
  int rc = parse_bitor_expression(cursor, program, &lhs, base_reg, line, diagnostic);
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
    rc = parse_bitor_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
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


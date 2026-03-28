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

static void copy_string_storage(char *dst, size_t capacity, const char *src) {
  size_t len;
  if (dst == NULL || capacity == 0U) {
    return;
  }
  if (src == NULL) {
    dst[0] = '\0';
    return;
  }
  len = strlen(src);
  if (len >= capacity) {
    len = capacity - 1U;
  }
  memcpy(dst, src, len);
  dst[len] = '\0';
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
  return strcmp(name, "print") == 0 || strcmp(name, "true") == 0 || strcmp(name, "false") == 0;
}

static void copy_name(char dst[GRAPHION_RUNTIME_NAME_MAX], const char *src) {
  size_t len = strlen(src);
  if (len >= GRAPHION_RUNTIME_NAME_MAX) {
    len = GRAPHION_RUNTIME_NAME_MAX - 1U;
  }
  memcpy(dst, src, len);
  dst[len] = '\0';
}

static int find_global_index_in_names(const char names[][GRAPHION_RUNTIME_NAME_MAX], size_t count, const char *name) {
  size_t i;
  for (i = 0U; i < count; ++i) {
    if (strcmp(names[i], name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int scope_find_index(const graphion_runtime_scope *scope, const char *name) {
  if (scope == NULL || name == NULL) {
    return -1;
  }
  return find_global_index_in_names(scope->global_names, scope->global_count, name);
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
  existing = find_global_index_in_names(program->global_names, program->global_count, name);
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
  char *dst;
  size_t len = 0U;
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
  dst = program->string_storage[program->const_count];
  while (**cursor != '\0' && **cursor != '"') {
    if (**cursor == '\n') {
      return fail(diagnostic, line, 1U, "unterminated string literal", GINT_ERR_PARSE);
    }
    if (len + 1U >= GRAPHION_RUNTIME_NAME_MAX * 4U) {
      return fail(diagnostic, line, 1U, "string literal too long", GINT_ERR_CAPACITY);
    }
    dst[len++] = **cursor;
    (*cursor)++;
  }
  if (**cursor != '"') {
    return fail(diagnostic, line, 1U, "unterminated string literal", GINT_ERR_PARSE);
  }
  (*cursor)++;
  dst[len] = '\0';
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
    double as_float;
    long long as_int;
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
      as_float = strtod(start, &end);
      if (end != scan) {
        return fail(diagnostic, line, 1U, "invalid float literal", GINT_ERR_PARSE);
      }
      value_out->kind = GVM_VALUE_FLOAT;
      value_out->as.float_value = as_float;
    } else {
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
  char name[GRAPHION_RUNTIME_NAME_MAX];
  int rc;
  int index;
  if (cursor == NULL || *cursor == NULL || program == NULL || operand_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  saved = *cursor;
  skip_spaces(cursor);
  if (is_ident_start_char(**cursor)) {
    rc = parse_identifier_token(cursor, name, sizeof(name), line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    if (strcmp(name, "true") == 0 || strcmp(name, "false") == 0) {
      *cursor = saved;
    } else {
      index = find_global_index_in_names(program->global_names, program->global_count, name);
      if (index < 0) {
        return fail(diagnostic, line, 1U, "unknown variable", GINT_ERR_UNKNOWN_VARIABLE);
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

static int parse_assignment(const char *line_text,
                            graphion_runtime_program *program,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = line_text;
  char target[GRAPHION_RUNTIME_NAME_MAX];
  parsed_operand operand;
  size_t target_index;
  int rc;

  rc = parse_identifier_token(&cursor, target, sizeof(target), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  if (is_reserved_name(target)) {
    return fail(diagnostic, line, 1U, "reserved name cannot be assigned", GINT_ERR_RESERVED_NAME);
  }
  skip_spaces(&cursor);
  if (*cursor != '=') {
    return fail(diagnostic, line, 1U, "expected '='", GINT_ERR_PARSE);
  }
  cursor++;
  rc = parse_operand(&cursor, program, &operand, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unsupported assignment expression", GINT_ERR_PARSE);
  }
  rc = program_find_or_add_global(program, target, line, diagnostic, &target_index);
  if (rc != GINT_OK) {
    return rc;
  }
  if (operand.kind == OPERAND_LITERAL) {
    return program_emit(program, GVM_OP_STORE_CONST_GLOBAL, 0U, (uint8_t)target_index, (int32_t)operand.const_index, line, diagnostic);
  }
  return program_emit(program, GVM_OP_COPY_GLOBAL, 0U, (uint8_t)target_index, (int32_t)operand.global_index, line, diagnostic);
}

static int parse_print(const char *line_text,
                       graphion_runtime_program *program,
                       unsigned int line,
                       graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = line_text;
  parsed_operand operand;
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
  rc = parse_operand(&cursor, program, &operand, line, diagnostic);
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
  if (operand.kind == OPERAND_LITERAL) {
    return program_emit(program, GVM_OP_PRINT_CONST, 0U, 0U, (int32_t)operand.const_index, line, diagnostic);
  }
  return program_emit(program, GVM_OP_PRINT_GLOBAL, 0U, 0U, (int32_t)operand.global_index, line, diagnostic);
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

static int parse_single_line(const char *line_text,
                             graphion_runtime_program *program,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic) {
  const char *line_cursor = line_text;
  int rc;
  skip_spaces(&line_cursor);
  if (*line_cursor == '\0') {
    return GINT_OK;
  }
  if (line_cursor != line_text) {
    return fail(diagnostic, line, 1U, "unexpected indentation", GINT_ERR_PARSE);
  }
  if (strncmp(line_cursor, "print", 5U) == 0 && !is_ident_char(line_cursor[5])) {
    rc = parse_print(line_cursor, program, line, diagnostic);
  } else {
    rc = parse_assignment(line_cursor, program, line, diagnostic);
  }
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_HALT, 0U, 0U, 0, line, diagnostic);
}

void graphion_runtime_scope_init(graphion_runtime_scope *scope) {
  size_t i;
  if (scope == NULL) {
    return;
  }
  scope->global_count = 0U;
  for (i = 0U; i < GRAPHION_RUNTIME_BINDING_MAX; ++i) {
    scope->global_names[i][0] = '\0';
    scope->string_storage[i][0] = '\0';
    vm_value_set_none(&scope->globals[i]);
  }
}

void graphion_runtime_scope_dispose(graphion_runtime_scope *scope) {
  graphion_runtime_scope_init(scope);
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
    vm_value_set_none(&program->const_pool[i]);
  }
  memset(program->program, 0, sizeof(program->program));
}

void graphion_runtime_program_dispose(graphion_runtime_program *program) {
  graphion_runtime_program_init(program);
}

int graphion_prepare_source(const char *source,
                            graphion_runtime_program *program,
                            graphion_runtime_diagnostic *diagnostic) {
  const char *line_start;
  const char *cursor;
  unsigned int line = 1U;
  int rc;
  if (source == NULL || program == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  clear_diagnostic(diagnostic);
  graphion_runtime_program_init(program);
  line_start = source;
  cursor = source;
  for (;;) {
    if (*cursor == '\n' || *cursor == '\0') {
      char line_buffer[512];
      size_t len = (size_t)(cursor - line_start);
      const char *line_cursor = line_buffer;
      if (len >= sizeof(line_buffer)) {
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      memcpy(line_buffer, line_start, len);
      line_buffer[len] = '\0';
      skip_spaces(&line_cursor);
      if (*line_cursor != '\0') {
        if (strncmp(line_cursor, "print", 5U) == 0 && !is_ident_char(line_cursor[5])) {
          rc = parse_print(line_cursor, program, line, diagnostic);
        } else {
          rc = parse_assignment(line_cursor, program, line, diagnostic);
        }
        if (rc != GINT_OK) {
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
    return rc;
  }
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

static void scope_stabilize_string_globals(graphion_runtime_scope *scope) {
  size_t i;
  if (scope == NULL) {
    return;
  }
  for (i = 0U; i < scope->global_count; ++i) {
    if (scope->globals[i].kind != GVM_VALUE_STRING) {
      continue;
    }
    copy_string_storage(scope->string_storage[i],
                        sizeof(scope->string_storage[i]),
                        scope->globals[i].as.string_value);
    scope->globals[i].as.string_value = scope->string_storage[i];
  }
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
  if (output != NULL) {
    graphion_vm_bind_output_sink(&vm, output);
  }
  rc = graphion_vm_load(&vm, program->program, program->program_len);
  if (rc != GVM_OK) {
    return fail(diagnostic, 1U, 1U, "failed to load VM program", GINT_ERR_PARSE);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    return fail(diagnostic, 1U, 1U, "failed to execute VM program", GINT_ERR_RUN);
  }
  scope_stabilize_string_globals(scope);
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
  graphion_runtime_program program;
  const char *line_start;
  const char *cursor;
  unsigned int line = 1U;
  int rc;
  if (source == NULL || scope == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  clear_diagnostic(diagnostic);
  line_start = source;
  cursor = source;
  for (;;) {
    if (*cursor == '\n' || *cursor == '\0') {
      char line_buffer[512];
      size_t len = (size_t)(cursor - line_start);
      if (len >= sizeof(line_buffer)) {
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      memcpy(line_buffer, line_start, len);
      line_buffer[len] = '\0';
      seed_program_from_scope(&program, scope);
      rc = parse_single_line(line_buffer, &program, line, diagnostic);
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
  return GINT_OK;
}

int graphion_interpret_source(const char *source,
                              graphion_runtime_scope *scope,
                              graphion_runtime_diagnostic *diagnostic) {
  return graphion_interpret_source_with_output(source, scope, diagnostic, stdout);
}

/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/stmt.h"
#include "runtime/interpreter/source.h"

int graphion_prepare_source(const char *source,
                            graphion_runtime_program *program,
                            graphion_runtime_diagnostic *diagnostic) {
  runtime_source_line lines[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t line_count = 0U;
  size_t i;
  int rc;

  if (source == NULL || program == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }

  clear_diagnostic(diagnostic);
  graphion_runtime_program_init(program);
  rc = split_source_lines(source, lines, GRAPHION_RUNTIME_PROGRAM_MAX, &line_count, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }

  for (i = 0U; i < line_count; ++i) {
    const char *line_cursor;

    if (line_is_blank(&lines[i])) {
      continue;
    }
    line_cursor = line_content(&lines[i]);
    if (strncmp(line_cursor, "print", 5U) == 0 && !is_ident_char(line_cursor[5])) {
      rc = parse_print(line_cursor, NULL, program, lines[i].line, diagnostic);
    } else {
      rc = parse_assignment(line_cursor, program, lines[i].line, diagnostic);
    }
    if (rc != GINT_OK) {
      return rc;
    }
  }

  return program_emit(program, GVM_OP_HALT, 0U, 0U, 0, line_count > 0U ? lines[line_count - 1U].line : 1U, diagnostic);
}

int graphion_interpret_source(const char *source,
                              graphion_runtime_scope *scope,
                              graphion_runtime_diagnostic *diagnostic) {
  return graphion_interpret_source_with_output(source, scope, diagnostic, stdout);
}

/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_SOURCE_H
#define GRAPHION_RUNTIME_INTERPRETER_SOURCE_H

#include "runtime/interpreter/expr.h"

typedef struct {
  char text[512];
  unsigned int line;
  unsigned int indent;
} runtime_source_line;

typedef struct {
  graphion_vm_value value;
  char *owned_string;
} runtime_match_case_value;

void runtime_match_case_value_init(runtime_match_case_value *entry);
void runtime_match_case_value_dispose(runtime_match_case_value *entry);
int runtime_match_case_value_clone(runtime_match_case_value *entry,
                                   const graphion_vm_value *value,
                                   unsigned int line,
                                   graphion_runtime_diagnostic *diagnostic);
int line_is_blank(const runtime_source_line *line);
const char *line_content(const runtime_source_line *line);
int line_starts_with_keyword(const runtime_source_line *line, const char *keyword);
int line_keyword_is_assignment_like(const runtime_source_line *line, const char *keyword);
int line_is_if_clause(const runtime_source_line *line);
int line_is_elif_clause(const runtime_source_line *line);
int line_is_else_clause(const runtime_source_line *line);
int line_is_match_clause(const runtime_source_line *line);
int line_is_default_clause(const runtime_source_line *line);
size_t find_next_nonblank_line(const runtime_source_line *lines, size_t count, size_t start);
size_t scan_block_end(const runtime_source_line *lines, size_t count, size_t start, unsigned int block_indent);
int split_source_lines(const char *source,
                       runtime_source_line *lines,
                       size_t capacity,
                       size_t *count_out,
                       graphion_runtime_diagnostic *diagnostic);
int condition_line_looks_incomplete(const char *text, size_t length);
int ternary_line_looks_incomplete(const char *text, size_t length);
int collect_control_condition_text(const runtime_source_line *lines,
                                   size_t count,
                                   size_t start_index,
                                   const char *keyword,
                                   char *buffer,
                                   size_t buffer_size,
                                   size_t *header_end_index_out,
                                   unsigned int line,
                                   graphion_runtime_diagnostic *diagnostic);
int parse_else_header(const char *cursor,
                      unsigned int line,
                      graphion_runtime_diagnostic *diagnostic);
int collect_match_expression_text(const runtime_source_line *lines,
                                  size_t count,
                                  size_t start_index,
                                  char *buffer,
                                  size_t buffer_size,
                                  size_t *header_end_index_out,
                                  unsigned int line,
                                  graphion_runtime_diagnostic *diagnostic);
int parse_match_case_header(const char *cursor,
                            graphion_vm_value *value_out,
                            runtime_match_case_value *owned_value,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic);
int parse_default_header(const char *cursor,
                         unsigned int line,
                         graphion_runtime_diagnostic *diagnostic);
const char *scalar_kind_name(const graphion_vm_value *value);
int copy_source_without_comments(const char *source,
                                 char **clean_out,
                                 graphion_runtime_diagnostic *diagnostic);
int process_file_level_directives(const char *source,
                                  graphion_runtime_warning_report *report,
                                  graphion_runtime_diagnostic *diagnostic);
int collect_match_warnings(const runtime_source_line *lines,
                           size_t count,
                           graphion_runtime_warning_report *report,
                           graphion_runtime_diagnostic *diagnostic);
int collect_graph_warnings(const runtime_source_line *lines,
                           size_t count,
                           graphion_runtime_warning_report *report,
                           graphion_runtime_diagnostic *diagnostic);

#endif

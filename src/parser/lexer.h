/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_PARSER_LEXER_H
#define GRAPHION_PARSER_LEXER_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  GLEX_OK = 0,
  GLEX_ERR_INVALID_ARG = -1,
  GLEX_ERR_CAPACITY = -2,
  GLEX_ERR_TOKEN = -3
} graphion_lexer_result;

typedef enum {
  GTOK_EOF = 0,
  GTOK_NEWLINE = 1,
  GTOK_COMMA = 2,
  GTOK_IDENTIFIER = 3,
  GTOK_REGISTER = 4,
  GTOK_INTEGER = 5
} graphion_token_kind;

typedef struct {
  uint8_t kind;
  size_t offset;
  size_t length;
  size_t line;
  size_t column;
  int64_t int_value;
  uint8_t reg_value;
} graphion_token;

int graphion_lex_source(const char *source,
                        graphion_token *out_tokens,
                        size_t out_capacity,
                        size_t *out_count);

#endif

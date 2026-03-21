/* SPDX-License-Identifier: MIT */

#include "parser/lexer.h"

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

static int push_token(graphion_token *out_tokens,
                      size_t out_capacity,
                      size_t *count,
                      uint8_t kind,
                      size_t offset,
                      size_t length,
                      size_t line,
                      size_t column,
                      int64_t int_value,
                      uint8_t reg_value) {
  if (*count >= out_capacity) {
    return GLEX_ERR_CAPACITY;
  }
  out_tokens[*count].kind = kind;
  out_tokens[*count].offset = offset;
  out_tokens[*count].length = length;
  out_tokens[*count].line = line;
  out_tokens[*count].column = column;
  out_tokens[*count].int_value = int_value;
  out_tokens[*count].reg_value = reg_value;
  ++(*count);
  return GLEX_OK;
}

static int lex_integer(const char *source,
                       size_t offset,
                       size_t line,
                       size_t column,
                       graphion_token *out_tokens,
                       size_t out_capacity,
                       size_t *count,
                       size_t *consumed) {
  char *end = NULL;
  long value = strtol(source + offset, &end, 10);
  size_t length;
  if (end == NULL || end == source + offset) {
    return GLEX_ERR_TOKEN;
  }
  if (value < (long)INT32_MIN || value > (long)INT32_MAX) {
    return GLEX_ERR_TOKEN;
  }
  length = (size_t)(end - (source + offset));
  *consumed = length;
  return push_token(out_tokens,
                    out_capacity,
                    count,
                    GTOK_INTEGER,
                    offset,
                    length,
                    line,
                    column,
                    (int64_t)value,
                    0U);
}

static int lex_identifier_or_register(const char *source,
                                      size_t offset,
                                      size_t line,
                                      size_t column,
                                      graphion_token *out_tokens,
                                      size_t out_capacity,
                                      size_t *count,
                                      size_t *consumed) {
  size_t length = 1U;
  while (source[offset + length] != '\0') {
    const unsigned char ch = (unsigned char)source[offset + length];
    if (isalnum(ch) == 0 && ch != '_') {
      break;
    }
    ++length;
  }
  *consumed = length;
  if (length >= 2U && (source[offset] == 'r' || source[offset] == 'R')) {
    size_t i;
    long reg_value = 0L;
    for (i = 1U; i < length; ++i) {
      const unsigned char ch = (unsigned char)source[offset + i];
      if (isdigit(ch) == 0) {
        reg_value = -1L;
        break;
      }
      reg_value = (reg_value * 10L) + (long)(ch - (unsigned char)'0');
      if (reg_value > 255L) {
        reg_value = -1L;
        break;
      }
    }
    if (reg_value >= 0L) {
      return push_token(out_tokens,
                        out_capacity,
                        count,
                        GTOK_REGISTER,
                        offset,
                        length,
                        line,
                        column,
                        0,
                        (uint8_t)reg_value);
    }
  }
  return push_token(out_tokens,
                    out_capacity,
                    count,
                    GTOK_IDENTIFIER,
                    offset,
                    length,
                    line,
                    column,
                    0,
                    0U);
}

int graphion_lex_source(const char *source,
                        graphion_token *out_tokens,
                        size_t out_capacity,
                        size_t *out_count) {
  size_t offset = 0U;
  size_t count = 0U;
  size_t line = 1U;
  size_t column = 1U;
  int rc;

  if (source == NULL || out_tokens == NULL || out_count == NULL) {
    return GLEX_ERR_INVALID_ARG;
  }

  while (source[offset] != '\0') {
    const unsigned char ch = (unsigned char)source[offset];
    if (ch == ' ' || ch == '\t' || ch == '\r') {
      ++offset;
      ++column;
      continue;
    }
    if (ch == '#') {
      while (source[offset] != '\0' && source[offset] != '\n') {
        ++offset;
        ++column;
      }
      continue;
    }
    if (ch == '/' && source[offset + 1U] == '/') {
      while (source[offset] != '\0' && source[offset] != '\n') {
        ++offset;
        ++column;
      }
      continue;
    }
    if (ch == '\n') {
      rc = push_token(out_tokens, out_capacity, &count, GTOK_NEWLINE, offset, 1U, line, column, 0, 0U);
      if (rc != GLEX_OK) {
        return rc;
      }
      ++offset;
      ++line;
      column = 1U;
      continue;
    }
    if (ch == ',') {
      rc = push_token(out_tokens, out_capacity, &count, GTOK_COMMA, offset, 1U, line, column, 0, 0U);
      if (rc != GLEX_OK) {
        return rc;
      }
      ++offset;
      ++column;
      continue;
    }
    if (isalpha(ch) != 0 || ch == '_') {
      size_t consumed = 0U;
      rc = lex_identifier_or_register(source,
                                      offset,
                                      line,
                                      column,
                                      out_tokens,
                                      out_capacity,
                                      &count,
                                      &consumed);
      if (rc != GLEX_OK) {
        return rc;
      }
      offset += consumed;
      column += consumed;
      continue;
    }
    if (isdigit(ch) != 0 || (ch == '-' && isdigit((unsigned char)source[offset + 1U]) != 0)) {
      size_t consumed = 0U;
      rc = lex_integer(source, offset, line, column, out_tokens, out_capacity, &count, &consumed);
      if (rc != GLEX_OK) {
        return rc;
      }
      offset += consumed;
      column += consumed;
      continue;
    }
    return GLEX_ERR_TOKEN;
  }

  rc = push_token(out_tokens, out_capacity, &count, GTOK_EOF, offset, 0U, line, column, 0, 0U);
  if (rc != GLEX_OK) {
    return rc;
  }
  *out_count = count;
  return GLEX_OK;
}

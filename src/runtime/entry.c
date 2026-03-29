/* SPDX-License-Identifier: MIT */

#include "runtime/entry.h"

#include <stdio.h>
#include <string.h>

enum {
  GENTRY_SOURCE_MAX = 16 * 1024
};

int graphion_source_path_is_gion(const char *path) {
  size_t len;
  if (path == NULL) {
    return 0;
  }
  len = strlen(path);
  return len >= 5U && strcmp(path + len - 5U, ".gion") == 0;
}

static int read_source_file(const char *path, char *buffer, size_t capacity) {
  FILE *fp;
  size_t read_len;
  if (path == NULL || buffer == NULL || capacity == 0U) {
    return GENTRY_ERR_INVALID_ARG;
  }
  fp = NULL;
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "rb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "rb");
#endif
  if (fp == NULL) {
    return GENTRY_ERR_IO;
  }
  read_len = fread(buffer, 1U, capacity - 1U, fp);
  if (ferror(fp) != 0) {
    fclose(fp);
    return GENTRY_ERR_IO;
  }
  if (!feof(fp)) {
    fclose(fp);
    return GENTRY_ERR_CAPACITY;
  }
  buffer[read_len] = '\0';
  fclose(fp);
  return GENTRY_OK;
}

int graphion_run_gion_path(const char *path,
                           graphion_runtime_scope *scope,
                           graphion_runtime_diagnostic *diagnostic) {
  char source[GENTRY_SOURCE_MAX];
  int rc;

  if (path == NULL || scope == NULL) {
    return GENTRY_ERR_INVALID_ARG;
  }
  if (!graphion_source_path_is_gion(path)) {
    return GENTRY_ERR_EXTENSION;
  }
  rc = read_source_file(path, source, sizeof(source));
  if (rc != GENTRY_OK) {
    return rc;
  }
  graphion_runtime_scope_init(scope);
  rc = graphion_interpret_source(source, scope, diagnostic);
  if (rc == GINT_ERR_PARSE || rc == GINT_ERR_RESERVED_NAME) {
    return GENTRY_ERR_PARSE;
  }
  if (rc != GINT_OK) {
    return GENTRY_ERR_RUN;
  }
  return GENTRY_OK;
}

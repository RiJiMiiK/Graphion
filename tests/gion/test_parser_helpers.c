/* SPDX-License-Identifier: MIT */

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "test_parser_helpers.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

int test_make_temp_path(char *buffer, size_t capacity, const char *label) {
  if (buffer == NULL || capacity == 0U) {
    return 0;
  }

#if defined(_WIN32)
  {
    char temp_dir[MAX_PATH];
    char temp_file[MAX_PATH];
    DWORD dir_len = GetTempPathA((DWORD)sizeof(temp_dir), temp_dir);
    int written;

    if (dir_len == 0U || dir_len >= sizeof(temp_dir)) {
      return 0;
    }
    if (GetTempFileNameA(temp_dir, "gio", 0U, temp_file) == 0U) {
      return 0;
    }
    remove(temp_file);
    written =
        snprintf(buffer, capacity, "%s%s%s", temp_file, label != NULL ? "_" : "", label != NULL ? label : "");
    return written > 0 && (size_t)written < capacity;
  }
#else
  {
    const char *tmp_dir = getenv("TMPDIR");
    char temp_template[256];
    int fd;
    int written;

    if (tmp_dir == NULL || *tmp_dir == '\0') {
      tmp_dir = "/tmp";
    }
    written = snprintf(temp_template, sizeof(temp_template), "%s/graphion_test_XXXXXX", tmp_dir);
    if (written <= 0 || (size_t)written >= sizeof(temp_template)) {
      return 0;
    }
    fd = mkstemp(temp_template);
    if (fd < 0) {
      return 0;
    }
    close(fd);
    remove(temp_template);
    written = snprintf(buffer, capacity, "%s%s%s", temp_template, label != NULL ? "_" : "", label != NULL ? label : "");
    return written > 0 && (size_t)written < capacity;
  }
#endif
}

FILE *test_open_temp_output(char *path_buffer, size_t capacity, const char *label) {
  FILE *fp = NULL;

  if (!test_make_temp_path(path_buffer, capacity, label)) {
    return NULL;
  }
#if defined(_MSC_VER)
  if (fopen_s(&fp, path_buffer, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path_buffer, "wb");
#endif
  return fp;
}

int finish_scope_test(graphion_runtime_scope *scope, int code) {
  graphion_runtime_scope_dispose(scope);
  return code;
}

int test_read_file_text(const char *path, char *buffer, size_t capacity) {
  FILE *fp = NULL;
  size_t read_len;
  if (path == NULL || buffer == NULL || capacity == 0U) {
    return 0;
  }
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "rb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "rb");
#endif
  if (fp == NULL) {
    return 0;
  }
  read_len = fread(buffer, 1U, capacity - 1U, fp);
  fclose(fp);
  buffer[read_len] = '\0';
  return 1;
}

int test_capture_gion_output(const char *source,
                             const char *label,
                             graphion_runtime_scope *scope,
                             graphion_runtime_diagnostic *diagnostic,
                             char *path_buffer,
                             size_t path_capacity,
                             char *output_buffer,
                             size_t output_capacity) {
  FILE *fp = NULL;
  int rc;

  if (source == NULL || scope == NULL || diagnostic == NULL || path_buffer == NULL || output_buffer == NULL ||
      path_capacity == 0U || output_capacity == 0U) {
    return 0;
  }
  path_buffer[0] = '\0';
  fp = test_open_temp_output(path_buffer, path_capacity, label);
  if (fp == NULL) {
    return 0;
  }
  rc = graphion_interpret_source_with_output(source, scope, diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    test_cleanup_temp_path(path_buffer);
    return 0;
  }
  if (!test_read_file_text(path_buffer, output_buffer, output_capacity)) {
    test_cleanup_temp_path(path_buffer);
    return 0;
  }
  return 1;
}

void test_cleanup_temp_path(const char *path) {
  if (path != NULL && *path != '\0') {
    remove(path);
  }
}

void normalize_text_newlines(char *text) {
  size_t read_index = 0U;
  size_t write_index = 0U;
  if (text == NULL) {
    return;
  }
  while (text[read_index] != '\0') {
    if (text[read_index] != '\r') {
      text[write_index++] = text[read_index];
    }
    ++read_index;
  }
  text[write_index] = '\0';
}

#pragma once

#include <stdio.h>
#include <uv.h>

#include "cJSON.h"

#ifndef NO_LOGGING
#define LOG_PROC(TYPE, FMT, ...) fprintf(stderr, "["#TYPE"] " FMT "\n", ##__VA_ARGS__)
#else
#define LOG_PROC()
#endif

#define LOG_DEBUG(FMT, ...) LOG_PROC(DEBUG, FMT, ##__VA_ARGS__)
#define LOG_TRACE(FMT, ...) LOG_PROC(TRACE, FMT, ##__VA_ARGS__)
#define LOG_INFO(FMT, ...) LOG_PROC(INFO, FMT, ##__VA_ARGS__)
#define LOG_WARN(FMT, ...) LOG_PROC(WARN, FMT, ##__VA_ARGS__)

#define JOBJ cJSON*

#define JSON(VARIABLE, STATUS, BLOCK) \
JOBJ VARIABLE = cJSON_CreateObject(); \
do {                                  \
    __label__ fail;                   \
    (STATUS) = true;                  \
    JOBJ _json_curr_ = NULL;          \
    BLOCK;                            \
    break;                            \
    fail:                             \
    cJSON_Delete(VARIABLE);           \
    (STATUS) = false;                 \
} while(0);

#define FIELD_OBJ(PARENT, NAME, BLOCK) (_json_curr_ = cJSON_AddObjectToObject((PARENT), (NAME))); \
do { \
if (_json_curr_ == NULL) { goto fail; } \
BLOCK \
} while(0)

#define FIELD_ARR(PARENT, NAME, BLOCK) (_json_curr_ = cJSON_AddArrayToObject((PARENT), (NAME))); \
do { \
if (_json_curr_ == NULL) { goto fail; } \
BLOCK \
} while(0)

#define FIELD_PRIM_CHECKED(PARENT, NAME, VAL, FUNCTION) (_json_curr_ = FUNCTION((PARENT), (NAME), (VAL))); \
do { \
if (_json_curr_ == NULL) { goto fail; } \
} while(0)

#define FIELD_STR(PARENT, NAME, STR) FIELD_PRIM_CHECKED(PARENT, NAME, STR, cJSON_AddStringToObject)
#define FIELD_NUM(PARENT, NAME, NUM) FIELD_PRIM_CHECKED(PARENT, NAME, NUM, cJSON_AddNumberToObject)
#define FIELD_BOOL(PARENT, NAME, BOOL) FIELD_PRIM_CHECKED(PARENT, NAME, BOOL, cJSON_AddBoolToObject)

#define SUB_OBJ(BLOCK) (_json_curr_ = cJSON_CreateObject()); \
do { \
if (_json_curr_ == NULL) { goto fail; } \
BLOCK \
} while(0)

#define ARR_ADD(ARR, ITEM) do { if (!cJSON_AddItemToArray((ARR), (ITEM))) {goto fail;} } while(0)

#define talloc(TYPE) malloc(sizeof(TYPE))
#define tzalloc(TYPE) calloc(1, sizeof(TYPE))

#define CHECK_UV(EXPR) do {                             \
    int r = (EXPR);                                     \
    if (r) {                                            \
        fprintf(stderr, "Error: %s\n", uv_strerror(r)); \
        abort();                                        \
    }                                                   \
} while(0) 

static void print_buf_as_hex(FILE* file, uv_buf_t buf) {
    for (int i = 0; i < buf.len; ++i) {
        fprintf(file, "%02X", (unsigned char)buf.base[i]);

        if (i < buf.len - 1) {
            fprintf(file, " ");
        }
    }
    fprintf(file, "\n");
}

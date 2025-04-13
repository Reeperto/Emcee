#pragma once

#include <stdint.h>

#define MAX_VARINT_LEN 5
#define VARINT_SEGMENT_BITS 0x7F
#define VARINT_CONTINUE_BIT 0x80

typedef struct {
    int byte_count;
    uint8_t bytes[MAX_VARINT_LEN];
} VarInt;

VarInt varint_from_int(int val);

typedef struct {
    const char* data;
    int len;
} String;

typedef struct {
    uint64_t inner[2];
} UUID;

UUID uuid_make_random();

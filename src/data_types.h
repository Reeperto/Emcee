#pragma once

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_VARINT_LEN 5
#define VARINT_SEGMENT_BITS 0x7F
#define VARINT_CONTINUE_BIT 0x80

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef struct {
    int byte_count;
    u8 bytes[MAX_VARINT_LEN];
} VarInt;

VarInt varint_from_int(int val);

typedef struct {
    const char* data;
    int len;
} String;

String string_from_cstr(const char* str);
#define CSTR_TO_STR(CSTR) string_from_cstr((CSTR))

void string_copy_to_cstr(char* dest, String src);

typedef struct {
    u64 inner[2];
} UUID;

UUID uuid_make_random();

typedef union {
    f64 a[3];
    struct {
        f64 x, y, z;
    };
} Vec3;

typedef struct {
    i32 x, y, z;
} Position;

static inline i8 deg_to_angle(f32 degrees) {
    return (i8)(fmodf(degrees, 360.0f) * (256.0f) / (360.0f));
}

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

typedef struct {
    int variant_count;
    uint8_t* bits;
} EnumSet;

#define BITSET_STORAGE_BYTES(num_bits) (((num_bits) + 7) / 8)
#define DEFINE_ENUMSET(name, nbits)                        \
    struct {                                               \
        EnumSet header;                                    \
        unsigned char storage[BITSET_STORAGE_BYTES(nbits)];\
    } name = { { (nbits), (name).storage }, {0} };

#define enumset_set(set, variant) _enumset_set((EnumSet*)(set), (int)(variant))
#define enumset_unset(set, variant) _enumset_unset((EnumSet*)(set), (int)(variant))
void _enumset_set(EnumSet* set, int variant);
void _enumset_unset(EnumSet* set, int variant);

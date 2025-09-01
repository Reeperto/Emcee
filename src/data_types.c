#include "data_types.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

VarInt varint_from_int(int val) {
    VarInt res = {0};

    int pos = 0;
    while (true) {
        if ((val & ~VARINT_SEGMENT_BITS) == 0) {
            res.bytes[pos] = val;
            break;
        }

        res.bytes[pos] = (val & VARINT_SEGMENT_BITS) | VARINT_CONTINUE_BIT;

        pos += 1;
        val >>= 7;
    }

    res.byte_count = pos + 1;

    return res;
}

String string_from_cstr(const char* str) {
    return (String){
        .data = str,
        .len = strlen(str)
    };
}

void string_copy_to_cstr(char* dest, String src) {
    memcpy(dest, src.data, src.len);
    dest[src.len] = '\0';
}

// TODO(eli): This doesnt actually follow the uuid spec AT ALL. This should be 
// replaced with a proper UUID generation mechanism.
UUID uuid_make_random() {
    return (UUID){
        .inner[0] = rand(),
        .inner[1] = rand(),
    };
}

void bitset_set(u8 *bits, int idx) {
    bits[idx / 8] |= (1 << (idx % 8));
}

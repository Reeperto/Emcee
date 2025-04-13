#include "data_types.h"

#include <stdbool.h>
#include <stdlib.h>

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

// TODO(eli): This doesnt actually follow the uuid spec AT ALL. This should be 
// replaced with a proper UUID generation mechanism.
UUID uuid_make_random() {
    return (UUID){
        .inner[0] = rand(),
        .inner[1] = rand(),
    };
}

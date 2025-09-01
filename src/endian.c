#include "endian.h"

bool IS_LITTLE_ENDIAN;

void init_endianess() {
    IS_LITTLE_ENDIAN = !(1 == htonl(1));
}

static inline u16 swap16(u16 val) {
    return (val << 8) | (val >> 8);
}

static inline u32 swap32(u32 val) {
    return ((val << 24) & 0xff000000) |
           ((val << 8)  & 0x00ff0000) |
           ((val >> 8)  & 0x0000ff00) |
           ((val >> 24) & 0x000000ff);
}

static inline u64 swap64(u64 val) {
    return ((val << 56) & 0xff00000000000000ULL) |
           ((val << 40) & 0x00ff000000000000ULL) |
           ((val << 24) & 0x0000ff0000000000ULL) |
           ((val << 8)  & 0x000000ff00000000ULL) |
           ((val >> 8)  & 0x00000000ff000000ULL) |
           ((val >> 24) & 0x0000000000ff0000ULL) |
           ((val >> 40) & 0x000000000000ff00ULL) |
           ((val >> 56) & 0x00000000000000ffULL);
}

inline u16 to_be16(u16 val) {
    return IS_LITTLE_ENDIAN ? swap16(val) : val;
}

inline u32 to_be32(u32 val) {
    return IS_LITTLE_ENDIAN ? swap32(val) : val;
}

inline u64 to_be64(u64 val) {
    return IS_LITTLE_ENDIAN ? swap64(val) : val;
}

inline u16 from_be16(u16 val) {
    return IS_LITTLE_ENDIAN ? swap16(val) : val;
}

inline u32 from_be32(u32 val) {
    return IS_LITTLE_ENDIAN ? swap32(val) : val;
}

inline u64 from_be64(u64 val) {
    return IS_LITTLE_ENDIAN ? swap64(val) : val;
}

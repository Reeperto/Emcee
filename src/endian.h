#pragma once

#include "data_types.h"

void init_endianness();

u16 to_be16(u16 val);
u32 to_be32(u32 val);
u64 to_be64(u64 val);
u16 from_be16(u16 val);
u32 from_be32(u32 val);
u64 from_be64(u64 val);

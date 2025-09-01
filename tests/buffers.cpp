#include "cJSON.h"
#include "uv.h"
#include <gtest/gtest.h>
#include <stdio.h>

extern "C" {
#include "packet.h"
}

extern "C" {
void __ubsan_on_report() {
  FAIL() << "Encountered an undefined behavior sanitizer error";
}
void __asan_on_error() {
  FAIL() << "Encountered an address sanitizer error";
}
void __tsan_on_report() {
  FAIL() << "Encountered a thread sanitizer error";
}
}  // extern "C"

TEST(BufferUtils, VarInt) {
    VarInt out;

    out = varint_from_int(0);
    EXPECT_EQ(out.byte_count, 1);
    EXPECT_EQ(out.bytes[0], 0x00);

    out = varint_from_int(1);
    EXPECT_EQ(out.byte_count, 1);
    EXPECT_EQ(out.bytes[0], 0x01);

    out = varint_from_int(2);
    EXPECT_EQ(out.byte_count, 1);
    EXPECT_EQ(out.bytes[0], 0x02);

    out = varint_from_int(127);
    EXPECT_EQ(out.byte_count, 1);
    EXPECT_EQ(out.bytes[0], 0x7f);

    out = varint_from_int(128);
    EXPECT_EQ(out.byte_count, 2);
    EXPECT_EQ(out.bytes[0], 0x80);
    EXPECT_EQ(out.bytes[1], 0x01);
}

TEST(PacketBuilder, EmptyPacket) {
    PacketBuilder pb = {0};
    uv_buf_t packet = pb_finalize(&pb);

    // packet.base = "\x00"
    // packet.len  = 1

    EXPECT_EQ(packet.len, 1);
    EXPECT_EQ(packet.base[0], '\x00');
}

TEST(PacketBuilder, DataTypes) {
    PacketBuilder pb = {0};

    pb_write_bool(&pb, false);
    uv_buf_t packet = pb_finalize(&pb);
    
    EXPECT_EQ(packet.len, 2);
    EXPECT_EQ(packet.base[0], '\x01');
    EXPECT_EQ(packet.base[1], '\x00');
}

TEST(PacketBuilder, JsonToNBT) {
    PacketBuilder pb = {0};

    const char* example_json = 
        R"({ 
            "asset_id": "minecraft:entity/cat/cat",
            "spawn_conditions": [
                { "priority": 0 }
            ]
        })";

    pb_nbt_from_json(&pb, cJSON_Parse(example_json));
}

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
    uint8_t varint[5] = {};
    int byte_count;

    convert_to_varint(0, varint, &byte_count);
    EXPECT_EQ(byte_count, 1);
    EXPECT_EQ(varint[0], 0x00);

    convert_to_varint(1, varint, &byte_count);
    EXPECT_EQ(byte_count, 1);
    EXPECT_EQ(varint[0], 0x01);

    convert_to_varint(2, varint, &byte_count);
    EXPECT_EQ(byte_count, 1);
    EXPECT_EQ(varint[0], 0x02);

    convert_to_varint(127, varint, &byte_count);
    EXPECT_EQ(byte_count, 1);
    EXPECT_EQ(varint[0], 0x7f);

    convert_to_varint(128, varint, &byte_count);
    EXPECT_EQ(byte_count, 2);
    EXPECT_EQ(varint[0], 0x80);
    EXPECT_EQ(varint[1], 0x01);
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

// TEST(PacketBuilder, JsonToNBT) {
//     PacketBuilder pb = {0};
//
//     const char* example_json = 
//         "{" 
//             "\"asset_id\": \"minecraft:entity/cat/cat\","
//             "\"spawn_conditions\": ["
//                 "{ \"priority\": 0 }"
//             "]"
//         "}"
//     ;
//
//     pb_json_to_nbt(&pb, cJSON_Parse(example_json));
// }

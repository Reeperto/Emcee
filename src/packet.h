#pragma once

#include <arpa/inet.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "cJSON.h"
#include "data_types.h"
#include "util.h"

typedef struct {
    int remaining_bytes;
    bool done_reading_size;
    int packet_size;
    int varint_pos;
    u8* packet_data;
    int packet_data_write_pos;
} PacketStream;

typedef struct {
    uv_buf_t buffer;
    int pos;
} PacketReader;

void pr_delete(PacketReader* pr);
PacketReader pr_from_uv(uv_buf_t buf);

void pr_read_copy(PacketReader* pr, void* dest, int count);

u8 pr_read_u8(PacketReader* pr);
u16 pr_read_u16(PacketReader* pr);
u64 pr_read_u64(PacketReader* pr);
i64 pr_read_i64(PacketReader* pr);
f32 pr_read_f32(PacketReader* pr);
f64 pr_read_f64(PacketReader* pr);

int32_t pr_read_varint(PacketReader* pr);
String pr_read_string(PacketReader* pr);
UUID pr_read_uuid(PacketReader* pr);
Position pr_read_position(PacketReader* pr);

typedef struct {
    u8* bytes;
    int cap;
    int pos;
} PacketBuilder;

PacketBuilder pb_with_cap(PacketBuilder* pb, int cap);
void pb_extend(PacketBuilder* pb, int len);
void pb_reset(PacketBuilder* pb);
void pb_delete(PacketBuilder* pb);

uv_buf_t pb_finalize(PacketBuilder* pb);
int pb_size(const PacketBuilder* pb);

void pb_write_copy(PacketBuilder* pb, const void* src, int count);

void pb_write_bool(PacketBuilder* pb, bool val);
void pb_write_u8(PacketBuilder* pb, u8 val);
void pb_write_i8(PacketBuilder* pb, i8 val);
void pb_write_u16(PacketBuilder* pb, u16 val);
void pb_write_i16(PacketBuilder* pb, i16 val);
void pb_write_u32(PacketBuilder* pb, u32 val);
void pb_write_i32(PacketBuilder* pb, i32 val);
void pb_write_u64(PacketBuilder* pb, u64 val);
void pb_write_i64(PacketBuilder* pb, i64 val);
void pb_write_f32(PacketBuilder* pb, f32 val);
void pb_write_f64(PacketBuilder* pb, f64 val);

void pb_write_varint(PacketBuilder* pb, int val);
void pb_write_id(PacketBuilder* pb, int id);
void pb_write_string_c(PacketBuilder* pb, const char* str);
void pb_write_string(PacketBuilder* pb, String str);
void pb_write_json(PacketBuilder* pb, cJSON* json);
void pb_write_uuid(PacketBuilder* pb, UUID uuid);
void pb_write_position(PacketBuilder* pb, Position pos);

typedef enum {
	TAG_End,
	TAG_Byte,
	TAG_Short,
	TAG_Int,
	TAG_Long,
	TAG_Float,
	TAG_Double,
	TAG_Byte_Array,
	TAG_String,
	TAG_List,
 	TAG_Compound,
 	TAG_Int_Array,
 	TAG_Long_Array,
} NBTTag;

void pb_nbt_end(PacketBuilder* pb);
void pb_nbt_byte(PacketBuilder* pb, i8 val, const char* f_name);
void pb_nbt_short(PacketBuilder* pb, i16 val, const char* f_name);
void pb_nbt_int(PacketBuilder* pb, i32 val, const char* f_name);
void pb_nbt_long(PacketBuilder* pb, i64 val, const char* f_name);
void pb_nbt_float(PacketBuilder* pb, f32 val, const char* f_name);
void pb_nbt_double(PacketBuilder* pb, f64 val, const char* f_name);
void pb_nbt_byte_array(PacketBuilder* pb, i8* list, int size, const char* f_name);
void pb_nbt_string(PacketBuilder* pb, String str, const char* f_name);
void pb_nbt_string_c(PacketBuilder* pb, const char* str, const char* f_name);
void pb_nbt_list(PacketBuilder* pb, NBTTag element_type, int size, const char* f_name);
void pb_nbt_compound(PacketBuilder* pb, const char* f_name);
void pb_nbt_int_array(PacketBuilder* pb, i32* list, int size, const char* f_name);
void pb_nbt_long_array(PacketBuilder* pb, i64* list, int size, const char* f_name);
void pb_nbt_from_json(PacketBuilder* pb, JOBJ json);

void send_finalized_packet(PacketBuilder* pb, uv_stream_t* handle, bool reset_pb);

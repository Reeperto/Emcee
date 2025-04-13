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
    uv_buf_t buffer;
    int pos;
} PacketReader;

void pr_delete(PacketReader* pr);
PacketReader pr_from_uv(uv_buf_t buf);

void pr_read_copy(PacketReader* pr, void* dest, int count);

uint8_t pr_read_u8(PacketReader* pr);
uint16_t pr_read_u16(PacketReader* pr);
uint64_t pr_read_u64(PacketReader* pr);
int64_t pr_read_i64(PacketReader* pr);

int32_t pr_read_varint(PacketReader* pr);
String pr_read_string(PacketReader* pr);
UUID pr_read_uuid(PacketReader* pr);

typedef struct {
    uint8_t* bytes;
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
void pb_write_u8(PacketBuilder* pb, uint8_t val);
void pb_write_i8(PacketBuilder* pb, int8_t val);
void pb_write_u16(PacketBuilder* pb, uint16_t val);
void pb_write_i16(PacketBuilder* pb, int16_t val);
void pb_write_u32(PacketBuilder* pb, uint32_t val);
void pb_write_i32(PacketBuilder* pb, int32_t val);
void pb_write_u64(PacketBuilder* pb, uint64_t val);
void pb_write_i64(PacketBuilder* pb, int64_t val);
void pb_write_f32(PacketBuilder* pb, float val);
void pb_write_f64(PacketBuilder* pb, double val);

void pb_write_varint(PacketBuilder* pb, int val);
void pb_write_id(PacketBuilder* pb, int id);
void pb_write_string_c(PacketBuilder* pb, const char* str);
void pb_write_string(PacketBuilder* pb, String str);
void pb_write_json(PacketBuilder* pb, cJSON* json);
void pb_write_uuid(PacketBuilder* pb, UUID uuid);

#define pb_write_enumset(pb, set) _pb_write_enumset((pb), (EnumSet*)(set))
void _pb_write_enumset(PacketBuilder* pb, EnumSet* set);

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
void pb_nbt_byte(PacketBuilder* pb, int8_t val, const char* f_name);
void pb_nbt_short(PacketBuilder* pb, int16_t val, const char* f_name);
void pb_nbt_int(PacketBuilder* pb, int32_t val, const char* f_name);
void pb_nbt_long(PacketBuilder* pb, int64_t val, const char* f_name);
void pb_nbt_float(PacketBuilder* pb, float val, const char* f_name);
void pb_nbt_double(PacketBuilder* pb, double val, const char* f_name);
void pb_nbt_byte_array(PacketBuilder* pb, int8_t* list, int size, const char* f_name);
void pb_nbt_string(PacketBuilder* pb, String str, const char* f_name);
void pb_nbt_string_c(PacketBuilder* pb, const char* str, const char* f_name);
void pb_nbt_list(PacketBuilder* pb, NBTTag element_type, int size, const char* f_name);
void pb_nbt_compound(PacketBuilder* pb, const char* f_name);
void pb_nbt_int_array(PacketBuilder* pb, int32_t* list, int size, const char* f_name);
void pb_nbt_long_array(PacketBuilder* pb, int64_t* list, int size, const char* f_name);
void pb_nbt_from_json(PacketBuilder* pb, JOBJ json);

void send_finalized_packet(PacketBuilder* pb, uv_stream_t* handle, bool should_close);

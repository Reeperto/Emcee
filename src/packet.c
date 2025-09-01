#include "packet.h"

#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "cJSON.h"
#include "data_types.h"

u32 host_float_to_net(f32 val) {
    u32 tmp;
    memcpy(&tmp, &val, sizeof(u32));
    return tmp;
}

u64 host_double_to_net(double val) {
    u64 tmp;
    memcpy(&tmp, &val, sizeof(u64));
    return tmp;
}

void pr_delete(PacketReader* pr) {
    free(pr->buffer.base);
    memset(pr, 0, sizeof(PacketReader));
}

PacketReader pr_from_uv(uv_buf_t buf) {
    return (PacketReader){
        .buffer = buf,
        .pos = 0
    };
}

static void pr_check_length(PacketReader* pr, int len) {
    assert(pr->pos + len <= pr->buffer.len);
}

void pr_read_copy(PacketReader* pr, void* dest, int count) {
    pr_check_length(pr, count);
    memcpy(dest, &pr->buffer.base[pr->pos], count);
    pr->pos += count;
}

u8 pr_read_u8(PacketReader* pr) {
    pr_check_length(pr, sizeof(u8));
    return pr->buffer.base[pr->pos++];
}

u16 pr_read_u16(PacketReader* pr) {
    pr_check_length(pr, sizeof(u16));
    u16 val;
    pr_read_copy(pr, &val, sizeof(u16));
    return ntohs(val);
}

u32 pr_read_u32(PacketReader* pr) {
    pr_check_length(pr, sizeof(u32));
    u32 val;
    pr_read_copy(pr, &val, sizeof(u32));
    return ntohl(val);
}

u64 pr_read_u64(PacketReader* pr) {
    pr_check_length(pr, sizeof(u64));
    u64 val;
    pr_read_copy(pr, &val, sizeof(u64));
    return ntohll(val);
}

i64 pr_read_i64(PacketReader* pr) {
    pr_check_length(pr, sizeof(i64));
    i64 val;
    pr_read_copy(pr, &val, sizeof(i64));
    return (i64)ntohll(val);
}

f32 pr_read_f32(PacketReader* pr) {
    f32 val;
    u32 backing = pr_read_u32(pr);
    memcpy(&val, &backing, sizeof(f32));
    return val;
}

f64 pr_read_f64(PacketReader* pr) {
    f64 val;
    u64 backing = pr_read_u64(pr);
    memcpy(&val, &backing, sizeof(f64));
    return val;
}

i32 pr_read_varint(PacketReader* pr) {
    i32 value = 0;
    int position = 0;
    u8 byte = 0;

    while (true) {
        byte = pr_read_u8(pr);
        value |= (byte & VARINT_SEGMENT_BITS) << position;

        if ((byte & VARINT_CONTINUE_BIT) == 0) {
            break;
        }

        position += 7;

        assert(position < 32);
    }

    return value;
}

String pr_read_string(PacketReader* pr) {
    int len = pr_read_varint(pr);
    pr_check_length(pr, len);

    String str = (String){
        .data = &pr->buffer.base[pr->pos],
        .len = len
    };

    pr->pos += len;

    return str;
}

UUID pr_read_uuid(PacketReader *pr) {
    UUID out;
    out.inner[0] = pr_read_u64(pr);
    out.inner[1] = pr_read_u64(pr);

    return out;
}

Position pr_read_position(PacketReader* pr) {
    i64 data = pr_read_i64(pr);

    return (Position){
        .x = data >> 38,
        .y = data << 52 >> 52,
        .z = data << 26 >> 38
    };
}

// -----------------------------------------------------------------------------
// Packet Builder
// -----------------------------------------------------------------------------

PacketBuilder pb_with_cap(PacketBuilder* pb, int cap) {
    return (PacketBuilder){
        .bytes = malloc(cap),
        .cap = cap,
        .pos = 0
    };
}

void pb_extend(PacketBuilder* pb, int len) {
    if (pb->pos + len + 1 > pb->cap) {
        pb->cap += len;
        pb->bytes = realloc(pb->bytes, pb->cap);
    }
}

void pb_reset(PacketBuilder* pb) {
    pb->pos = 0;
}

void pb_delete(PacketBuilder* pb) {
    free(pb->bytes);
    memset(pb, 0, sizeof(PacketBuilder));
}

int pb_size(const PacketBuilder *pb) {
    return pb->pos;
}

// TODO(eli): Write tests for this :|
uv_buf_t pb_finalize(PacketBuilder* pb) {
    pb_extend(pb, 0); // Ensure packet is in a valid memory state

    VarInt packet_size = varint_from_int(pb->pos);

    u8* packet = malloc(packet_size.byte_count + pb->pos);
    memcpy(packet, packet_size.bytes, packet_size.byte_count);
    memcpy(packet + packet_size.byte_count, pb->bytes, pb->pos);

    return uv_buf_init((char*)packet, pb->pos + packet_size.byte_count);
}

void pb_write_copy(PacketBuilder* pb, const void* src, int count) {
    pb_extend(pb, count);
    memcpy(pb->bytes + pb->pos, src, count);
    pb->pos += count;
}

void pb_write_bool(PacketBuilder* pb, bool val) {
    pb_write_u8(pb, (u8)val);
}

void pb_write_u8(PacketBuilder* pb, u8 val) {
    pb_extend(pb, sizeof(u8));
    pb->bytes[pb->pos++] = val;
}

void pb_write_i8(PacketBuilder* pb, i8 val) {
    pb_extend(pb, sizeof(i8));
    pb->bytes[pb->pos++] = (u8)(val);
}

void pb_write_u16(PacketBuilder* pb, u16 val) {
    u16 big_end = htons(val);
    pb_write_copy(pb, &big_end, sizeof(u16));
}

void pb_write_i16(PacketBuilder* pb, i16 val) {
    i16 big_end = htons(val);
    pb_write_copy(pb, &big_end, sizeof(i16));
}

void pb_write_u32(PacketBuilder* pb, u32 val) {
    u32 big_end = htonl(val);
    pb_write_copy(pb, &big_end, sizeof(u32));
}

void pb_write_i32(PacketBuilder* pb, i32 val) {
    i32 big_end = htonl(val);
    pb_write_copy(pb, &big_end, sizeof(i32));
}

void pb_write_u64(PacketBuilder* pb, u64 val) {
    u64 big_end = htonll(val);
    pb_write_copy(pb, &big_end, sizeof(u64));
}

void pb_write_i64(PacketBuilder* pb, i64 val) {
    i64 big_end = htonll(val);
    pb_write_copy(pb, &big_end, sizeof(i64));
}

void pb_write_f32(PacketBuilder* pb, f32 val) {
    u32 data = host_float_to_net(val);
    pb_write_u32(pb, data);
}

void pb_write_f64(PacketBuilder* pb, f64 val) {
    u64 data = host_double_to_net(val);
    pb_write_u64(pb, data);
}

void pb_write_varint(PacketBuilder* pb, int val) {
    VarInt var_val = varint_from_int(val);

    pb_extend(pb, var_val.byte_count);
    pb_write_copy(pb, var_val.bytes, var_val.byte_count);
}

void pb_write_id(PacketBuilder* pb, int id) {
    pb_write_varint(pb, id);
}

void pb_write_string_c(PacketBuilder* pb, const char* str) {
    int len = strlen(str);
    pb_write_varint(pb, len);
    pb_write_copy(pb, str, len);
}

void pb_write_string(PacketBuilder* pb, String str) {
    pb_write_varint(pb, str.len);
    pb_write_copy(pb, str.data, str.len);
}

void pb_write_json(PacketBuilder* pb, cJSON* json) {
    char* json_str = cJSON_PrintUnformatted(json);
    assert(json_str != NULL);

    int json_len = strlen(json_str);

    pb_write_varint(pb, json_len);
    pb_write_copy(pb, json_str, json_len);
    free(json_str);
}

void pb_write_uuid(PacketBuilder* pb, UUID uuid) {
    pb_write_u64(pb, uuid.inner[0]);
    pb_write_u64(pb, uuid.inner[1]);
}

void pb_write_position(PacketBuilder* pb, Position pos) {
    i64 x = pos.x;
    i64 y = pos.y;
    i64 z = pos.z;

    i64 encoded = ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF);

    pb_write_u64(pb, encoded);
}

void pb_nbt_write_name(PacketBuilder* pb, const char* f_name) {
    if (f_name && strcmp(f_name, "") != 0) {
        int f_len = strlen(f_name);
        pb_write_u16(pb, f_len);
        pb_write_copy(pb, f_name, f_len);
    }
}

void pb_nbt_end(PacketBuilder* pb) {
    pb_write_u8(pb, TAG_End);
}

void pb_nbt_byte(PacketBuilder* pb, i8 val, const char* f_name) {
    pb_write_u8(pb, TAG_Byte);
    pb_nbt_write_name(pb, f_name);
    pb_write_i8(pb, val);
}

void pb_nbt_int(PacketBuilder* pb, i32 val, const char* f_name) {
    pb_write_u8(pb, TAG_Int);
    pb_nbt_write_name(pb, f_name);
    pb_write_i32(pb, val);
}

void pb_nbt_double(PacketBuilder* pb, f64 val, const char* f_name) {
    pb_write_u8(pb, TAG_Double);
    pb_nbt_write_name(pb, f_name);
    pb_write_f64(pb, val);
}

void pb_nbt_string(PacketBuilder* pb, String str, const char* f_name) {
    pb_write_u8(pb, TAG_String);
    pb_nbt_write_name(pb, f_name);
    pb_write_u16(pb, str.len);
    pb_write_copy(pb, str.data, str.len);
}

void pb_nbt_string_c(PacketBuilder* pb, const char* str, const char* f_name) {
    pb_nbt_string(pb, (String){ .data = str, .len = strlen(str) }, f_name);
}

void pb_nbt_list(PacketBuilder* pb, NBTTag element_type, int size, const char* f_name) {
    pb_write_u8(pb, TAG_List);
    pb_nbt_write_name(pb, f_name);
    pb_write_u8(pb, element_type);
    pb_write_i32(pb, size);
}

void pb_nbt_compound(PacketBuilder* pb, const char* f_name) {
    pb_write_u8(pb, TAG_Compound);
    pb_nbt_write_name(pb, f_name);
}

void pb_json_to_nbt_recur(PacketBuilder* pb, JOBJ json) {
    JOBJ e;

    cJSON_ArrayForEach(e, json) {
        switch (e->type) {
            case cJSON_String: {
                pb_nbt_string_c(pb, e->valuestring, e->string);
                break;
            }
            case cJSON_Array: {
                if (cJSON_IsObject(e->child)) {
                    pb_nbt_list(pb, TAG_Compound, cJSON_GetArraySize(e), e->string);
                    pb_json_to_nbt_recur(pb, e);
                } else {
                    assert(false);
                }

                break;
            }
            case cJSON_Object: {
                pb_nbt_compound(pb, e->string);
                pb_json_to_nbt_recur(pb, e);
                pb_nbt_end(pb);

                break;
            }
            case cJSON_Number: {
                if (floor(e->valuedouble) == e->valuedouble) {
                    i32 int_val = e->valuedouble;
                    pb_nbt_int(pb, int_val, e->string);
                } else {
                    pb_nbt_double(pb, e->valuedouble, e->string);
                }

                break;
            }
            case cJSON_True: 
            case cJSON_False: {
                pb_nbt_byte(pb, cJSON_IsTrue(e) ? 1 : 0, e->string);
                break;
            }
            default: {
                fprintf(stderr, "%d\n", e->type);
                assert(false);
            }
        }
    };
}

void pb_nbt_from_json(PacketBuilder* pb, JOBJ json) {
    pb_nbt_compound(pb, NULL);
    pb_json_to_nbt_recur(pb, json);
    pb_nbt_end(pb);
}

typedef struct {
    uv_write_t req;
    uv_buf_t packet_buf;
} PacketSentWrapper;

static void packet_sent_cb(uv_write_t* packet_write, int status) {
    free(((PacketSentWrapper*)(packet_write))->packet_buf.base);
    free(packet_write);
}

void send_finalized_packet(PacketBuilder* pb, uv_stream_t* handle, bool reset_pb) {
    uv_buf_t packet_data = pb_finalize(pb);

    if (reset_pb) {
        pb_reset(pb);
    }

    PacketSentWrapper* write_info = talloc(PacketSentWrapper);
    write_info->packet_buf = packet_data;

    uv_write((uv_write_t*)write_info, handle, &packet_data, 1, packet_sent_cb);
}

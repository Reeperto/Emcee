#include "packet.h"

#include <stdlib.h>
#include <math.h>

#include "cJSON.h"
#include "client.h"
#include "data_types.h"

uint32_t host_float_to_net(float val) {
    uint32_t tmp;
    memcpy(&tmp, &val, sizeof(uint32_t));
    return tmp;
}

float net_float_to_host(uint32_t val) {
    uint32_t host = ntohl(val);
    float out;
    memcpy(&out, &host, sizeof(float));
    return out;
}

uint64_t host_double_to_net(double val) {
    uint64_t tmp;
    memcpy(&tmp, &val, sizeof(uint64_t));
    return tmp;
}

double net_double_to_host(uint64_t val) {
    uint64_t host = ntohll(val);
    double out;
    memcpy(&out, &host, sizeof(double));
    return out;
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

uint8_t pr_read_u8(PacketReader* pr) {
    pr_check_length(pr, sizeof(uint8_t));
    return pr->buffer.base[pr->pos++];
}

uint16_t pr_read_u16(PacketReader* pr) {
    pr_check_length(pr, sizeof(uint16_t));
    uint16_t val;
    pr_read_copy(pr, &val, 2);
    return ntohs(val);
}

int64_t pr_read_i64(PacketReader* pr) {
    pr_check_length(pr, sizeof(int64_t));
    int64_t val;
    pr_read_copy(pr, &val, sizeof(int64_t));
    return (int64_t)ntohll(val);
}

uint64_t pr_read_u64(PacketReader* pr) {
    pr_check_length(pr, sizeof(uint64_t));
    uint64_t val;
    pr_read_copy(pr, &val, sizeof(uint64_t));
    return ntohll(val);
}

int32_t pr_read_varint(PacketReader* pr) {
    int32_t value = 0;
    int position = 0;
    uint8_t byte = 0;

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

    uint8_t* packet = malloc(packet_size.byte_count + pb->pos);
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
    if (val) {
        pb_write_u8(pb, 1);
    } else {
        pb_write_u8(pb, 0);
    }
}

void pb_write_u8(PacketBuilder* pb, uint8_t val) {
    pb_extend(pb, sizeof(uint8_t));
    pb->bytes[pb->pos++] = val;
}

void pb_write_i8(PacketBuilder* pb, int8_t val) {
    pb_extend(pb, sizeof(int8_t));
    pb->bytes[pb->pos++] = (uint8_t)(val);
}

void pb_write_u16(PacketBuilder* pb, uint16_t val) {
    uint16_t big_end = htons(val);
    pb_write_copy(pb, &big_end, sizeof(uint16_t));
}

void pb_write_i16(PacketBuilder* pb, int16_t val) {
    int16_t big_end = htons(val);
    pb_write_copy(pb, &big_end, sizeof(int16_t));
}

void pb_write_u32(PacketBuilder* pb, uint32_t val) {
    uint32_t big_end = htonl(val);
    pb_write_copy(pb, &big_end, sizeof(uint32_t));
}

void pb_write_i32(PacketBuilder* pb, int32_t val) {
    int32_t big_end = htonl(val);
    pb_write_copy(pb, &big_end, sizeof(int32_t));
}

void pb_write_u64(PacketBuilder* pb, uint64_t val) {
    uint64_t big_end = htonll(val);
    pb_write_copy(pb, &big_end, sizeof(uint64_t));
}

void pb_write_i64(PacketBuilder* pb, int64_t val) {
    int64_t big_end = htonll(val);
    pb_write_copy(pb, &big_end, sizeof(int64_t));
}

void pb_write_f32(PacketBuilder* pb, float val) {
    uint32_t data = host_float_to_net(val);
    pb_write_u32(pb, data);
}

void pb_write_f64(PacketBuilder* pb, double val) {
    uint64_t data = host_double_to_net(val);
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

void pb_nbt_end(PacketBuilder* pb) {
    pb_write_u8(pb, TAG_End);
}

void pb_nbt_byte(PacketBuilder* pb, int8_t val, const char* f_name) {
    pb_write_u8(pb, TAG_Byte);

    if (f_name && strcmp(f_name, "") != 0) {
        int f_len = strlen(f_name);
        pb_write_u16(pb, f_len);
        pb_write_copy(pb, f_name, f_len);
    }

    pb_write_i8(pb, val);
}

void pb_nbt_int(PacketBuilder* pb, int32_t val, const char* f_name) {
    pb_write_u8(pb, TAG_Int);

    if (f_name && strcmp(f_name, "") != 0) {
        int f_len = strlen(f_name);
        pb_write_u16(pb, f_len);
        pb_write_copy(pb, f_name, f_len);
    }

    pb_write_i32(pb, val);
}

void pb_nbt_double(PacketBuilder* pb, double val, const char* f_name) {
    pb_write_u8(pb, TAG_Double);

    if (f_name && strcmp(f_name, "") != 0) {
        int f_len = strlen(f_name);
        pb_write_u16(pb, f_len);
        pb_write_copy(pb, f_name, f_len);
    }

    pb_write_f64(pb, val);
}

void pb_nbt_string(PacketBuilder* pb, String str, const char* f_name) {
    pb_write_u8(pb, TAG_String);

    if (f_name && strcmp(f_name, "") != 0) {
        int f_len = strlen(f_name);
        pb_write_u16(pb, f_len);
        pb_write_copy(pb, f_name, f_len);
    }

    pb_write_u16(pb, str.len);
    pb_write_copy(pb, str.data, str.len);
}

void pb_nbt_string_c(PacketBuilder* pb, const char* str, const char* f_name) {
    pb_nbt_string(pb, (String){ .data = str, .len = strlen(str) }, f_name);
}

void pb_nbt_list(PacketBuilder* pb, NBTTag element_type, int size, const char* f_name) {
    pb_write_u8(pb, TAG_List);

    if (f_name && strcmp(f_name, "") != 0) {
        int f_len = strlen(f_name);
        pb_write_u16(pb, f_len);
        pb_write_copy(pb, f_name, f_len);
    }

    pb_write_u8(pb, element_type);
    pb_write_i32(pb, size);
}

void pb_nbt_compound(PacketBuilder* pb, const char* f_name) {
    pb_write_u8(pb, TAG_Compound);

    if (f_name && strcmp(f_name, "") != 0) {
        int f_len = strlen(f_name);
        pb_write_u16(pb, f_len);
        pb_write_copy(pb, f_name, f_len);
    }
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
                    int32_t int_val = e->valuedouble;
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
    LOG_TRACE("JSON_PAYLOAD = %s", cJSON_Print(json));
    pb_json_to_nbt_recur(pb, json);
    pb_nbt_end(pb);
}

typedef struct {
    uv_buf_t buf;
    bool close_connection;
} packet_sent_data_t;

static void packet_sent_cb(uv_write_t* packet_write, int status) {
    packet_sent_data_t* data = packet_write->data;

    if (data->close_connection) {
        uv_close((uv_handle_t*)packet_write->handle, client_close_connection_cb);
    }

    free(data->buf.base);
    free(data);
    free(packet_write);
}

void send_finalized_packet(PacketBuilder* pb, uv_stream_t* handle, bool should_close) {
    packet_sent_data_t* data = talloc(packet_sent_data_t);

    uv_buf_t packet_data = pb_finalize(pb);
    pb_reset(pb);

    data->buf = packet_data;
    data->close_connection = should_close;

    uv_write_t* write_req = talloc(uv_write_t);
    write_req->data = data;

    uv_write(write_req, handle, &packet_data, 1, packet_sent_cb);
}

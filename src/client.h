#pragma once

#include <uv.h>

#include "packet.h"

// Make the enum be a bitfield, that way we can define a table that matches a 
// given protocol ID and state to function.
typedef enum {
    HANDSHAKE,
    STATUS,
    LOGIN,
    TRANSFER,
    CONFIG,
    PLAY,
} ClientState;

typedef struct {
    uv_buf_t read_buffer;
    uv_stream_t* client_stream;
    uv_timer_t heartbeat;

    struct { 
        int read_left;
        bool done_reading_size;
        int packet_size;
        int varint_pos;
        uint8_t* packet_data;
        int packet_data_write_pos;
    } ri;

    ClientState state;
    PacketBuilder pb;
} ClientData;

void client_close_connection_cb(uv_handle_t* handle);
void client_send_heartbeat_cb(uv_timer_t* handle);

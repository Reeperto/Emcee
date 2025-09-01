#pragma once

#include <uv.h>

#include "data_types.h"
#include "packet.h"

typedef struct {
    u32 eid;

    Vec3 pos;
} ClientPlayer;

typedef enum {
    HANDSHAKE,
    STATUS,
    LOGIN,
    TRANSFER,
    CONFIG,
    PLAY,
} NetClientState;

#define MAX_USERNAME_LEN 16

typedef struct {
    uv_buf_t read_buffer;
    uv_stream_t* client_stream;
    uv_timer_t heartbeat;

    NetClientState state;

    PacketStream ps;
    PacketBuilder pb;

    char username[MAX_USERNAME_LEN + 1];
    UUID uuid;
    ClientPlayer player;
} NetClientData;

void net_client_close_connection_cb(uv_handle_t* handle);
void net_client_send_heartbeat_cb(uv_timer_t* handle);

#pragma once

#include "packet.h"
#include "client.h"
#include "packet_types.h"

typedef void (*PacketHandler)(uv_stream_t* handle, ClientData* client, PacketReader* pr, PacketBuilder* pb);
#define HANDLER_NAME(STATE, RESOURCE) packet_##STATE##_##RESOURCE
#define HANDLER(STATE, RESOURCE) void HANDLER_NAME(STATE, RESOURCE)(uv_stream_t* handle, ClientData* client, PacketReader* pr, PacketBuilder* pb)

#define HANDSHAKE_PACKETS \
    X(P_H_SB_INTENTION, intention)

#define STATUS_PACKETS \
    X(P_S_SB_STATUS_REQUEST, status_request) \
    X(P_S_SB_PING_REQUEST,   ping_request  )

#define LOGIN_PACKETS \
    X(P_L_SB_HELLO,              hello             ) \
    X(P_L_SB_LOGIN_ACKNOWLEDGED, login_acknowledged)

#define CONFIG_PACKETS \
    X(P_C_SB_SELECT_KNOWN_PACKS,   select_known_packs) \
    X(P_C_SB_FINISH_CONFIGURATION, finish_configuration)

#define PLAY_PACKETS \
    X(P_P_SB_CHAT_COMMAND, chat_command) 

#define X(ID, RESOURCE) HANDLER(HANDSHAKE, RESOURCE);
    HANDSHAKE_PACKETS
#undef X
static const PacketHandler HANDSHAKE_HANDLERS[0xFF] = {
#define X(ID, RESOURCE) [ID] = HANDLER_NAME(HANDSHAKE, RESOURCE),
    HANDSHAKE_PACKETS
#undef X
};

#define X(ID, RESOURCE) HANDLER(STATUS, RESOURCE);
    STATUS_PACKETS
#undef X
static const PacketHandler STATUS_HANDLERS[0xFF] = {
#define X(ID, RESOURCE) [ID] = HANDLER_NAME(STATUS, RESOURCE),
    STATUS_PACKETS
#undef X
};

#define X(ID, RESOURCE) HANDLER(LOGIN, RESOURCE);
    LOGIN_PACKETS
#undef X
static const PacketHandler LOGIN_HANDLERS[0xFF] = {
#define X(ID, RESOURCE) [ID] = HANDLER_NAME(LOGIN, RESOURCE),
    LOGIN_PACKETS
#undef X
};

#define X(ID, RESOURCE) HANDLER(CONFIG, RESOURCE);
    CONFIG_PACKETS
#undef X
static const PacketHandler CONFIG_HANDLERS[0xFF] = {
#define X(ID, RESOURCE) [ID] = HANDLER_NAME(CONFIG, RESOURCE),
    CONFIG_PACKETS
#undef X
};

#define X(ID, RESOURCE) HANDLER(PLAY, RESOURCE);
    PLAY_PACKETS
#undef X
static const PacketHandler PLAY_HANDLERS[0xFF] = {
#define X(ID, RESOURCE) [ID] = HANDLER_NAME(PLAY, RESOURCE),
    PLAY_PACKETS
#undef X
};

static inline void process_packet(int packet_id, uv_stream_t* handle, ClientData* client, PacketReader* pr, PacketBuilder* pb) {
    PacketHandler handler = NULL;

    switch (client->state) {
        case HANDSHAKE:
            handler = HANDSHAKE_HANDLERS[packet_id];
            break;
        case STATUS:
            handler = STATUS_HANDLERS[packet_id];
            break;
        case LOGIN:
            handler = LOGIN_HANDLERS[packet_id];
            break;
        case TRANSFER:
            assert(false);
            break;
        case CONFIG:
            handler = CONFIG_HANDLERS[packet_id];
            break;
        case PLAY:
            handler = PLAY_HANDLERS[packet_id];
            break;
    }

    if (handler != NULL) {
        handler(handle, client, pr, pb);
    }
}

#pragma once

#include "packet.h"
#include "client.h"
#include "util.h"

typedef void (*PacketHandler)(uv_stream_t* handle, Client* client, PacketReader* pr, PacketBuilder* pb);
#define HANDLER(STATE, RESOURCE) void packet_##STATE##_##RESOURCE(uv_stream_t* handle, Client* client, PacketReader* pr, PacketBuilder* pb)

HANDLER(HANDSHAKE, intention);

HANDLER(STATUS, status_request);
HANDLER(STATUS, ping_request);

HANDLER(LOGIN, hello);
HANDLER(LOGIN, login_acknowledged);

HANDLER(CONFIG, select_known_packs);
HANDLER(CONFIG, finish_configuration);

static void null_handler(uv_stream_t* handle, Client* client, PacketReader* pr, PacketBuilder* pb) {
    LOG_TRACE("Ignored packet");
}

static const PacketHandler HANDSHAKE_HANDLERS[0xFF] = {
    [0x00] = packet_HANDSHAKE_intention
};

static const PacketHandler STATUS_HANDLERS[0xFF] = {
    [0x00] = packet_STATUS_status_request,
    [0x01] = packet_STATUS_ping_request,
};

static const PacketHandler LOGIN_HANDLERS[0xFF] = {
    [0x00] = packet_LOGIN_hello,
    [0x03] = packet_LOGIN_login_acknowledged,
};

static const PacketHandler CONFIG_HANDLERS[0xFF] = {
    [0x00] = null_handler,
    [0x02] = null_handler,
    [0x03] = packet_CONFIG_finish_configuration,
    [0x07] = packet_CONFIG_select_known_packs,
};

#pragma once

#include <stdlib.h>
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
    ClientState state;
    PacketBuilder pb;
} Client;

static void client_close_connection_cb(uv_shutdown_t* req, int status) {
    Client* client = (Client*)(req->handle->data);

    pb_delete(&client->pb);
    free(client->read_buffer.base);

    free(req);
}

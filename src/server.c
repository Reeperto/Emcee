#include "server.h"

EmceeServer g_server = {0};

void server_init(EmceeServer* server) {
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        server->clients[i].free = true;
    }
}

NetClientData* server_new_client(EmceeServer* server) {
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (server->clients[i].free) {
            server->clients[i].free = false;
            NetClientData* client = &server->clients[i].client;
            memset(client, 0, sizeof(NetClientData));
            return client;
        }
    }

    ASSERT(false);
}

void server_free_client(EmceeServer* server, NetClientData* client) {
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (client == &server->clients[i].client) {
            server->clients[i].free = true;
            return;
        }
    }

    ASSERT(false);
}

void server_broadcast(EmceeServer* server, NetClientData* from, PacketBuilder* pb, bool reset_pb) {
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (server->clients[i].free || from == &server->clients[i].client) {
            continue;
        }
        
        send_finalized_packet(pb, server->clients[i].client.client_stream, false);
    }

    if (reset_pb) {
        pb_reset(pb);
    }
}

int server_get_eid(EmceeServer* server) {
    return server->next_eid++;
}

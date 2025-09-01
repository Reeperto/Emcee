#pragma once

#include <stdint.h>

#include "client.h"
#include "packet.h"

#define MAX_PLAYERS 5

typedef struct {
    // NOTE(eli): Lets just hope we dont exceed 2^32 entities spawned in the 
    // lifetime of the server :)
    u32 next_eid;

    struct {
        NetClientData client;
        bool free;
    } clients[MAX_PLAYERS];
} EmceeServer;

extern EmceeServer g_server;

void server_init(EmceeServer* server);
NetClientData* server_new_client(EmceeServer* server);
void server_free_client(EmceeServer* server, NetClientData* client);
void server_broadcast(EmceeServer* server, NetClientData* from, PacketBuilder* pb, bool reset_pb);
int server_get_eid(EmceeServer* server);

#define FOR_EACH_CLIENT(SERVER, FROM, CLIENT, BLOCK) \
for (int i = 0; i < MAX_PLAYERS; ++i) {                                        \
    if ((SERVER)->clients[i].free || (FROM) == &(SERVER)->clients[i].client) { \
        continue;                                                              \
    }                                                                          \
    (CLIENT) = &(SERVER)->clients[i].client;                                   \
    BLOCK                                                                      \
}                                                                              \

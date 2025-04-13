#include "packet_handlers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "cJSON.h"
#include "packet.h"
#include "packet_types.h"
#include "registry_data.h"
#include "util.h"

// -----------------------------------------------------------------------------
// HANDSHAKE
// -----------------------------------------------------------------------------

HANDLER(HANDSHAKE, intention) {
    int proto_ver = pr_read_varint(pr);
    String server_addr = pr_read_string(pr);
    uint16_t port = pr_read_u16(pr);
    int next_state = pr_read_varint(pr);

    LOG_TRACE(
        "C -> S: intention = { proto_ver = %d, server_addr = %.*s, server_port = %d, next_state = %d }", 
        proto_ver,
        server_addr.len,
        server_addr.data,
        port,
        next_state
    );

    client->state = next_state;
}

// -----------------------------------------------------------------------------
// STATUS
// -----------------------------------------------------------------------------

HANDLER(STATUS, status_request) {
    LOG_TRACE("C -> S: status_request = {}");

    const char* name = "Cabbit";
    const char* id = "4566e69f-c907-48ee-8d71-d7ba5aa00d20";

    bool json_ok;
    JSON(response, json_ok, {
        JOBJ version = FIELD_OBJ(response, "version", {
            FIELD_STR(version, "name", "1.21.5");
            FIELD_NUM(version, "protocol", 770);
        });

        JOBJ players = FIELD_OBJ(response, "players", {
            FIELD_NUM(players, "max", 3);
            FIELD_NUM(players, "online", 2);

            JOBJ sample = FIELD_ARR(players, "sample", {
                for (int i = 0; i < 2; ++i) {
                    JOBJ player = SUB_OBJ({
                        FIELD_STR(player, "name", name);
                        FIELD_STR(player, "id", id);
                    });

                    ARR_ADD(sample, player);
                }
            });
        });

        JOBJ desc = FIELD_OBJ(response, "description", {
            FIELD_STR(desc, "text", "c is knocking");
        });

        FIELD_BOOL(response, "enforcesSecureChat", false);
    });

    assert(json_ok);

    LOG_TRACE("S -> C: status_response");
    pb_write_id(pb, P_STATUS_RESPONSE);
    pb_write_json(pb, response);

    uv_buf_t packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);

    cJSON_Delete(response);
}

HANDLER(STATUS, ping_request) {
    int64_t ping_val = pr_read_i64(pr);
    LOG_TRACE("C -> S: ping_request = { Timestamp = %lld }", ping_val);

    LOG_TRACE("S -> C: pong_response");
    pb_write_id(pb, P_PONG_RESPONSE);
    pb_write_i64(pb, ping_val);

    uv_buf_t packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);

    client->state = HANDSHAKE;
}

// -----------------------------------------------------------------------------
// LOGIN
// -----------------------------------------------------------------------------

HANDLER(LOGIN, hello) {
    String username = pr_read_string(pr);
    UUID uuid = pr_read_uuid(pr);

    LOG_TRACE("C -> S: hello = { username = %.*s, uuid = %llX%llX }", 
              username.len, username.data, 
              uuid.inner[0], uuid.inner[1]);

    LOG_TRACE("S -> C: login_finished");
    pb_write_id(pb, P_LOGIN_FINISHED);
    pb_write_uuid(pb, uuid);
    pb_write_string(pb, username);
    pb_write_varint(pb, 0);

    uv_buf_t packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
}

HANDLER(LOGIN, login_acknowledged) {
    LOG_TRACE("C -> S: login_acknowledged");
    client->state = CONFIG;

    LOG_TRACE("S -> C: select_known_packs");
    pb_write_id(pb, P_SELECT_KNOWN_PACKS);
    pb_write_varint(pb, 1);
    pb_write_string_c(pb, "minecraft");
    pb_write_string_c(pb, "core");
    pb_write_string_c(pb, "1.21.5");

    uv_buf_t packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
}

// -----------------------------------------------------------------------------
// CONFIG
// -----------------------------------------------------------------------------

HANDLER(CONFIG, finish_configuration) {
    LOG_TRACE("C -> S: finish_configuration");
    client->state = PLAY;

    uv_timer_start(&client->heartbeat, client_send_heartbeat_cb, 2000, 2000);

    LOG_TRACE("S -> C: login");
    pb_write_id(pb, P_LOGIN);

    pb_write_u32(pb, 0);                          // Entity ID
    pb_write_bool(pb, false);                     // Hardcore?

    pb_write_varint(pb, 1);                       // Dimension names array
    pb_write_string_c(pb, "minecraft:overworld"); // ...Dimension name

    pb_write_varint(pb, 3);                       // Max Players
    pb_write_varint(pb, 16);                      // View Distance
    pb_write_varint(pb, 16);                      // Simulation Distance
    pb_write_bool(pb, false);                     // Reduced Debug?
    pb_write_bool(pb, true);                      // Respawn Screen?
    pb_write_bool(pb, false);                     // Limited Crafting?
    pb_write_varint(pb, 0);                       // Dimension ID
    pb_write_string_c(pb, "minecraft:overworld"); // Dimension
    pb_write_i64(pb, 0);                          // Hashed seed
    pb_write_u8(pb, 1);                           // Gamemode
    pb_write_i8(pb, -1);                          // Previous Gamemode
    pb_write_bool(pb, false);                     // World Debug?
    pb_write_bool(pb, false);                     // World Superflat?
    pb_write_bool(pb, false);                     // Death Location?
    pb_write_varint(pb, 0);                       // Portal Cooldown
    pb_write_varint(pb, 54);                      // Sea Level
    pb_write_bool(pb, false);                     // Secure Chat?

    uv_buf_t packet = pb_finalize(pb);
    print_buf_as_hex(stderr, packet);
    send_finalized_packet(handle, packet, false);
    pb_reset(pb);

    LOG_TRACE("S -> C: game_event");
    pb_write_id(pb, P_GAME_EVENT);
    pb_write_u8(pb, 13);
    pb_write_f32(pb, 0);

    packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
    pb_reset(pb);

    LOG_TRACE("S -> C: set_chunk_cache_center");
    pb_write_id(pb, P_SET_CHUNK_CACHE_CENTER);
    pb_write_varint(pb, 0);
    pb_write_varint(pb, 0);
    
    packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
    pb_reset(pb);

    LOG_TRACE("S -> C: player_position");
    pb_write_id(pb, P_PLAYER_POSITION);
    pb_write_varint(pb, 0);   // Teleport ID
    pb_write_f64(pb, 8);      // Pos X
    pb_write_f64(pb, 128);    // Pos Y
    pb_write_f64(pb, 8);      // Pos Z

    pb_write_f64(pb, 0);      // Vel X
    pb_write_f64(pb, 0);      // Vel Y
    pb_write_f64(pb, 0);      // Vel Z

    pb_write_f32(pb, 0);      // Yaw
    pb_write_f32(pb, 0);      // Pitch

    pb_write_u32(pb, 0);      // Teleport Flags

    packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
    pb_reset(pb);

    LOG_TRACE("S -> C: level_chunk_with_light");
    pb_write_id(pb, P_LEVEL_CHUNK_WITH_LIGHT);

    pb_write_i32(pb, 0); // Chunk X
    pb_write_i32(pb, 0); // Chunk Z

    pb_write_varint(pb, 0); // Heightmap Count

    PacketBuilder pb_chunk = {0};
    
    // --- Full chunk
    for (int i = 1; i <= 24; ++i) {
        if (i <= 12) {
            pb_write_i16(&pb_chunk, 4096);
            pb_write_u8(&pb_chunk, 0);
            pb_write_varint(&pb_chunk, i == 12 ? 8 : 1);
        } else {
            pb_write_i16(&pb_chunk, 0);
            pb_write_u8(&pb_chunk, 0);
            pb_write_varint(&pb_chunk, 0);
        }

        pb_write_u8(&pb_chunk, 0);
        pb_write_varint(&pb_chunk, 0);
    }

    assert(pb_chunk.pos == (2+2+2) * 24);

    pb_write_varint(pb, pb_chunk.pos); // Data Size
    pb_write_copy(pb, pb_chunk.bytes, pb_chunk.pos);

    pb_delete(&pb_chunk);

    pb_write_varint(pb, 0); // Skylight Mask
    pb_write_varint(pb, 0); // Block Light Mask
    pb_write_varint(pb, 0); // Empty Sky Light Mask
    pb_write_varint(pb, 0); // Empty Block Light Mask

    pb_write_varint(pb, 0); // Sky Light Arrays 
    pb_write_varint(pb, 0); // Block Light Arrays 

    pb_write_varint(pb, 0); // Block Entity Data Array

    packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
    pb_reset(pb);
}

HANDLER(CONFIG, select_known_packs) {
    LOG_TRACE("C -> S: select_known_packs");

    uv_buf_t packet;

    for (int i = 0; registries[i].registry_name != NULL; ++i) {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, registries[i].registry_name);
        
        int entry_count = 0;
        while (registries[i].entries[entry_count]) {
            ++entry_count;
        }

        LOG_TRACE("Sending registry %s with %d entries", 
                  registries[i].registry_name,
                  entry_count);

        pb_write_varint(pb, entry_count);
        
        for (int j = 0; registries[i].entries[j] != NULL; ++j) {
            pb_write_string_c(pb, registries[i].entries[j]);
            pb_write_bool(pb, false);
        }

        packet = pb_finalize(pb);
        print_buf_as_hex(stderr, packet);
        fprintf(stderr, "\n");

        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    LOG_TRACE("S -> C: finish_configuration");
    pb_write_id(pb, P_FINISH_CONFIGURATION);
    packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
}

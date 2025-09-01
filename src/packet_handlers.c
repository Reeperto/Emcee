#include "packet_handlers.h"

#include <string.h>
#include <uv.h>

#include "cJSON.h"
#include "data_types.h"
#include "packet.h"
#include "packet_types.h"
#include "registry_data.h"
#include "server.h"
#include "util.h"

// -----------------------------------------------------------------------------
// HANDSHAKE
// -----------------------------------------------------------------------------

HANDLER(HANDSHAKE, intention) {
    int proto_ver = pr_read_varint(pr);
    String server_addr = pr_read_string(pr);
    u16 port = pr_read_u16(pr);
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
    pb_write_id(pb, P_S_CB_STATUS_RESPONSE);
    pb_write_json(pb, response);

    send_finalized_packet(pb, handle, true);

    cJSON_Delete(response);
}

HANDLER(STATUS, ping_request) {
    i64 ping_val = pr_read_i64(pr);
    LOG_TRACE("C -> S: ping_request = { Timestamp = %lld }", ping_val);

    LOG_TRACE("S -> C: pong_response");
    pb_write_id(pb, P_S_CB_PONG_RESPONSE);
    pb_write_i64(pb, ping_val);

    send_finalized_packet(pb, handle, true);

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

    client->uuid = uuid_make_random();
    string_copy_to_cstr(client->username, username);

    LOG_TRACE("S -> C: login_finished");
    P_L_CB_Login_Finished login_finished = {
        .uuid = uuid,
        .username = username,
    };

    P_L_CB_Login_Finished_write(pb, &login_finished);
    send_finalized_packet(pb, handle, true);
}

HANDLER(LOGIN, login_acknowledged) {
    LOG_TRACE("C -> S: login_acknowledged");
    client->state = CONFIG;

    LOG_TRACE("S -> C: select_known_packs");
    P_C_CB_Select_Known_Packs packs = {
        .entries_count = 1,
        .entries = ANON_ARRAY(packs.entries){
            {
                CSTR_TO_STR("minecraft"),
                CSTR_TO_STR("core"),
                CSTR_TO_STR("1.21.5")
            }
        }
    };

    P_C_CB_Select_Known_Packs_write(pb, &packs);
    send_finalized_packet(pb, handle, true);
}

// -----------------------------------------------------------------------------
// CONFIG
// -----------------------------------------------------------------------------

static void single_player_info(PacketBuilder* pb, void* data) {
    struct { UUID uuid; String username; }* player_info = data;

    pb_write_varint(pb, 1);
    pb_write_uuid(pb, player_info->uuid);
    pb_write_string(pb, player_info->username);
    pb_write_varint(pb, 0);
}

HANDLER(CONFIG, finish_configuration) {
    LOG_TRACE("C -> S: finish_configuration");
    client->state = PLAY;

    uv_timer_start(&client->heartbeat, net_client_send_heartbeat_cb, 2000, 2000);

    LOG_TRACE("S -> C: login");
    P_P_CB_Login login_packet = {
        .eid = (client->player.eid = server_get_eid(&g_server)),
        .hardcore = false,
        .dimension_names = {
            .count = 1,
            .elems = ANON_ARRAY(login_packet.dimension_names.elems){
                CSTR_TO_STR("minecraft:overworld")
            }
        },
        .max_players = 10,
        .view_distance = 16,
        .simulation_distance = 16,
        .reduced_debug = false,
        .respawn_screen = true,
        .limited_crafting = false,
        .dimension_id = 0,
        .dimension_name = string_from_cstr("minecraft:overworld"),
        .hashed_seed = 0,
        .gamemode = 1,
        .prev_gamemode = -1,
        .debug_mode = false,
        .superflat = false,
        .death_location.present = false,
        .portal_cooldown = 0,
        .sea_level = 64,
        .enforce_secure_chat = false
    };

    P_P_CB_Login_write(pb, &login_packet);
    send_finalized_packet(pb, handle, true);

    CommandNode nodes[] = {
        (CommandNode){
            .type = CN_ROOT,
            .children = {
                .count = 1,
                .elems = (int[]){1},
            }
        },
        (CommandNode){
            .type = CN_LITERAL,
            .children = {
                .count = 1,
                .elems = (int[]){2},
            },
            .data.literal = {
                .name = CSTR_TO_STR("lua"),
            }
        },
        (CommandNode){
            .type = CN_ARGUMENT,
            .data.argument = {
                .name = CSTR_TO_STR("cmd"),
                .parser_id = 19 // Message parser
            }
        }
    };

    P_P_CB_Commands commands = {
        .root_idx = 0,
        .node_count = 3,
        .nodes = nodes
    };

    P_P_CB_Commands_write(pb, &commands);
    send_finalized_packet(pb, handle, true);

    LOG_TRACE("S -> C: game_event");
    pb_write_id(pb, P_P_CB_GAME_EVENT);
    pb_write_u8(pb, 13);
    pb_write_f32(pb, 0);

    send_finalized_packet(pb, handle, true);

    LOG_TRACE("S -> C: set_chunk_cache_center");
    pb_write_id(pb, P_P_CB_SET_CHUNK_CACHE_CENTER);
    pb_write_varint(pb, 0);
    pb_write_varint(pb, 0);
    
    send_finalized_packet(pb, handle, true);

    client->player.pos = (Vec3){
        .x = 8,
        .y = 128,
        .z = 8
    };

    LOG_TRACE("S -> C: player_position");
    P_P_CB_Player_Position player_pos = {
        .teleport_id = rand(),
        .x = client->player.pos.x,
        .y = client->player.pos.y,
        .z = client->player.pos.z,
    };

    P_P_CB_Player_Position_write(pb, &player_pos);
    send_finalized_packet(pb, handle, true);

    LOG_TRACE("S -> C: level_chunk_with_light");
    pb_write_id(pb, P_P_CB_LEVEL_CHUNK_WITH_LIGHT);

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
        pb_write_varint(&pb_chunk, 27);
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

    send_finalized_packet(pb, handle, true);

    P_P_CB_Player_Info_Update player_info = {
        .action_set = PA_ADD_PLAYER,
        .players = {
            .proc = single_player_info,
            .data = &(struct { UUID uuid; String username; }){
                .uuid = client->uuid,
                .username = CSTR_TO_STR(client->username)
            }
        }
    };

    P_P_CB_Player_Info_Update_write(pb, &player_info);
    server_broadcast(&g_server, client, pb, true);

    P_P_CB_Add_Entity spawn_player = {
        .eid = client->player.eid,
        .uuid = client->uuid,
        .entity_type = ET_PLAYER,
        .x = 8,
        .y = 128,
        .z = 8,
    };

    P_P_CB_Add_Entity_write(pb, &spawn_player);
    server_broadcast(&g_server, client, pb, true);

    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (g_server.clients[i].free || &g_server.clients[i].client == client) {
            continue;
        }

        NetClientData* other_client = &g_server.clients[i].client;

        P_P_CB_Player_Info_Update other_player_info = {
            .action_set = PA_ADD_PLAYER,
            .players = {
                .proc = single_player_info,
                .data = &(struct { UUID uuid; String username; }){
                    .uuid = other_client->uuid,
                    .username = CSTR_TO_STR(other_client->username)
                }
            }
        };

        P_P_CB_Player_Info_Update_write(pb, &other_player_info);
        send_finalized_packet(pb, handle, true);

        P_P_CB_Add_Entity spawn_other_player = {
            .eid = other_client->player.eid,
            .uuid = other_client->uuid,
            .entity_type = ET_PLAYER,
            .x = other_client->player.pos.x,
            .y = other_client->player.pos.y,
            .z = other_client->player.pos.z,
        };

        P_P_CB_Add_Entity_write(pb, &spawn_other_player);
        send_finalized_packet(pb, handle, true);
    }
}

static void generate_registry_data_array(PacketBuilder* pb, void* data) {
    struct RegistryPair* registry = data;

    int entry_count = 0;
    while (registry->entries[entry_count]) {
        ++entry_count;
    }

    pb_write_varint(pb, entry_count);
    
    for (int j = 0; registry->entries[j] != NULL; ++j) {
        pb_write_string_c(pb, registry->entries[j]);
        pb_write_bool(pb, false);
    }
}

HANDLER(CONFIG, select_known_packs) {
    for (int i = 0; registries[i].registry_name != NULL; ++i) {
        P_L_CB_Registry_Data registry_packet = {
            .registry_id = string_from_cstr(registries[i].registry_name),
            .entries = {
                .data = &registries[i],
                .proc = generate_registry_data_array
            }
        };

        P_L_CB_Registry_Data_write(pb, &registry_packet);
        send_finalized_packet(pb, handle, true);
    }

    P_C_CB_Finish_Configuration finish_config = {};
    P_C_CB_Finish_Configuration_write(pb, &finish_config);
    send_finalized_packet(pb, handle, true);
}

// -----------------------------------------------------------------------------
// PLAY
// -----------------------------------------------------------------------------

HANDLER(PLAY, move_player_pos) {
    f64 x_new = pr_read_f64(pr);
    f64 y_new = pr_read_f64(pr);
    f64 z_new = pr_read_f64(pr);

    f64 d_x = x_new - client->player.pos.x;
    f64 d_y = y_new - client->player.pos.y;
    f64 d_z = z_new - client->player.pos.z;

    i16 d_fixed_x = (i16)(d_x * (1 << 12));
    i16 d_fixed_y = (i16)(d_y * (1 << 12));
    i16 d_fixed_z = (i16)(d_z * (1 << 12));

    client->player.pos.x = x_new;
    client->player.pos.y = y_new;
    client->player.pos.z = z_new;

    pb_write_varint(pb, 0x2E);
    pb_write_varint(pb, client->player.eid);
    pb_write_i16(pb, d_fixed_x);
    pb_write_i16(pb, d_fixed_y);
    pb_write_i16(pb, d_fixed_z);
    pb_write_bool(pb, true);

    server_broadcast(&g_server, client, pb, true);
}

HANDLER(PLAY, move_player_pos_rot) {
    f64 x_new = pr_read_f64(pr);
    f64 y_new = pr_read_f64(pr);
    f64 z_new = pr_read_f64(pr);

    f32 yaw = pr_read_f32(pr);
    f32 pitch = pr_read_f32(pr);

    f64 d_x = x_new - client->player.pos.x;
    f64 d_y = y_new - client->player.pos.y;
    f64 d_z = z_new - client->player.pos.z;

    i16 d_fixed_x = (i16)(d_x * (1 << 12));
    i16 d_fixed_y = (i16)(d_y * (1 << 12));
    i16 d_fixed_z = (i16)(d_z * (1 << 12));

    client->player.pos.x = x_new;
    client->player.pos.y = y_new;
    client->player.pos.z = z_new;

    i8 yaw_new = deg_to_angle(yaw);
    i8 pitch_new = deg_to_angle(pitch);

    pb_write_varint(pb, 0x2F);
    pb_write_varint(pb, client->player.eid);
    pb_write_i16(pb, d_fixed_x);
    pb_write_i16(pb, d_fixed_y);
    pb_write_i16(pb, d_fixed_z);
    pb_write_i8(pb, yaw_new);
    pb_write_i8(pb, pitch_new);
    pb_write_bool(pb, true);

    server_broadcast(&g_server, client, pb, true);

    pb_write_varint(pb, 0x4C);
    pb_write_varint(pb, client->player.eid);
    pb_write_i8(pb, yaw_new);

    server_broadcast(&g_server, client, pb, true);
}

HANDLER(PLAY, move_player_rot) {
    f32 yaw = pr_read_f32(pr);
    f32 pitch = pr_read_f32(pr);

    pb_write_id(pb, 0x31);
    pb_write_varint(pb, client->player.eid);
    pb_write_i8(pb, deg_to_angle(yaw));
    pb_write_i8(pb, deg_to_angle(pitch));
    pb_write_bool(pb, false);

    server_broadcast(&g_server, client, pb, true);
}

HANDLER(PLAY, player_action) {
    int status = pr_read_varint(pr);
    Position pos = pr_read_position(pr);
    pr_read_u8(pr);

    int block_seq = pr_read_varint(pr);

    P_P_CB_Block_Changed_Ack acknowledge = {
        .sequence_id = block_seq
    };

    P_P_CB_Block_Changed_Ack_write(pb, &acknowledge);
    send_finalized_packet(pb, handle, true);

    if (status == 0) {
        LOG_TRACE("Block broken");
        pb_write_id(pb, 0x08);
        pb_write_position(pb, pos);
        pb_write_varint(pb, 0);
        send_finalized_packet(pb, handle, true);
    }
}

static void add_friend_array_filler(PacketBuilder* pb, void* data) {
    pb_write_varint(pb, 1);
    pb_write_uuid(pb, *(UUID*)data);
    pb_write_string_c(pb, "friend :3");
    pb_write_varint(pb, 0);
}

HANDLER(PLAY, chat_command) {
    String cmd = pr_read_string(pr);
    LOG_TRACE("Recieved player command -> '%.*s'", cmd.len, cmd.data);

    if (strncmp(cmd.data, "inv", 3) == 0) {
        pb_write_id(pb, P_P_CB_OPEN_SCREEN);
        pb_write_varint(pb, 1);
        pb_write_varint(pb, 0);

        pb_nbt_compound(pb, NULL);
        pb_nbt_string_c(pb, ":3", "text");
        pb_nbt_end(pb);

        send_finalized_packet(pb, handle, true);
    } else if (strncmp(cmd.data, "friend", 6) == 0) {
        LOG_INFO("Spawning a friend :)");

        UUID friend_uuid = uuid_make_random();

        P_P_CB_Player_Info_Update player_info = {
            .players = {
                .proc = add_friend_array_filler,
                .data = &friend_uuid,
            },
            .action_set = PA_ADD_PLAYER
        };

        P_P_CB_Player_Info_Update_write(pb, &player_info);
        send_finalized_packet(pb, handle, true);

        P_P_CB_Add_Entity friend_entity = {
            .eid = server_get_eid(&g_server),
            .uuid = friend_uuid,
            .entity_type = ET_PLAYER,
            .x = 8, 
            .y = 128,
            .z = 8,
        };

        P_P_CB_Add_Entity_write(pb, &friend_entity);
        send_finalized_packet(pb, handle, true);
    }
}

HANDLER(PLAY, chat_command_signed) {
}

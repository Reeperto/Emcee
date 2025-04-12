#include "packet_handlers.h"

#include <stdlib.h>
#include <uv.h>

#include "cJSON.h"
#include "packet.h"
#include "packet_types.h"
#include "util.h"

typedef struct {
    uv_buf_t buf;
    bool close_connection;
} packet_sent_data_t;

static void packet_sent_cb(uv_write_t* packet_write, int status) {
    packet_sent_data_t* data = packet_write->data;

    if (data->close_connection) {
        uv_shutdown(talloc(uv_shutdown_t), packet_write->handle, client_close_connection_cb);
    }

    free(data->buf.base);
    free(data);
}

void send_finalized_packet(uv_stream_t* handle, uv_buf_t packet, bool should_close) {
    packet_sent_data_t* data = talloc(packet_sent_data_t);
    data->buf = packet;
    data->close_connection = should_close;

    uv_write_t* write_req = talloc(uv_write_t);
    write_req->data = data;

    uv_write(write_req, handle, &packet, 1, packet_sent_cb);
}

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

    pb_write_id(pb, P_STATUS_RESPONSE);
    pb_write_json(pb, response);

    uv_buf_t packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
    LOG_TRACE("S -> C: status_response");

    cJSON_Delete(response);
}

HANDLER(STATUS, ping_request) {
    int64_t ping_val = pr_read_i64(pr);
    LOG_TRACE("C -> S: ping_request = { Timestamp = %lld }", ping_val);

    pb_write_id(pb, P_PONG_RESPONSE);
    pb_write_i64(pb, ping_val); // Payload

    uv_buf_t packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, true);
    LOG_TRACE("S -> C: pong_response");

    // XXX(eli): Closing the connection here means that the pong response may just die.

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

    pb_write_id(pb, P_LOGIN_FINISHED);
    pb_write_uuid(pb, uuid);
    pb_write_string(pb, username);
    pb_write_varint(pb, 0);

    uv_buf_t packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
    LOG_TRACE("S -> C: login_finished");
}

HANDLER(LOGIN, login_acknowledged) {
    LOG_TRACE("C -> S: login_acknowledged");
    client->state = CONFIG;

    pb_write_id(pb, P_SELECT_KNOWN_PACKS);
    pb_write_varint(pb, 1);
    pb_write_string_c(pb, "minecraft");
    pb_write_string_c(pb, "core");
    pb_write_string_c(pb, "1.21.5");

    uv_buf_t packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
    LOG_TRACE("S -> C: select_known_packs");
}

// -----------------------------------------------------------------------------
// CONFIG
// -----------------------------------------------------------------------------

HANDLER(CONFIG, finish_configuration) {
    LOG_TRACE("C -> S: finish_configuration");
    client->state = PLAY;

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
    pb_write_varint(pb, 0);                       // Sea Level
    pb_write_bool(pb, false);                     // Secure Chat?


    uv_buf_t packet = pb_finalize(pb);
    print_buf_as_hex(stderr, packet);
    send_finalized_packet(handle, packet, false);
    LOG_TRACE("S -> C: login");
}

HANDLER(CONFIG, select_known_packs) {
    LOG_TRACE("C -> S: select_known_packs");

    uv_buf_t packet;

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "worldgen/biome");
        pb_write_varint(pb, 1);
        pb_write_string_c(pb, "plains");
        pb_write_bool(pb, true);

        pb_nbt_from_json(pb, cJSON_Parse(
        "{"
        "\"downfall\": 0.4000000059604645,"
        "\"effects\": {"
        "    \"fog_color\": 12638463,"
        "    \"mood_sound\": {"
        "        \"block_search_extent\": 8,"
        "        \"offset\": 2.0,"
        "        \"sound\": \"minecraft:ambient.cave\","
        "        \"tick_delay\": 6000"
        "    },"
        "    \"sky_color\": 7907327,"
        "    \"water_color\": 4159204,"
        "    \"water_fog_color\": 329011"
        "},"
        "\"has_precipitation\": true,"
        "\"temperature\": 0.800000011920929"
        "}"
        ));

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "damage_type");
        pb_write_varint(pb, 0);

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "chat_type");
        pb_write_varint(pb, 0);

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "dimension_type");
        pb_write_varint(pb, 1);

        pb_write_string_c(pb, "overworld");
        pb_write_bool(pb, true);
        pb_nbt_from_json(pb, cJSON_Parse(
        "{"
        "    \"ambient_light\": 0.0,"
        "    \"bed_works\": true,"
        "    \"coordinate_scale\": 1.0,"
        "    \"effects\": \"minecraft:overworld\","
        "    \"has_ceiling\": false,"
        "    \"has_raids\": true,"
        "    \"has_skylight\": true,"
        "    \"height\": 384,"
        "    \"infiniburn\": \"#minecraft:infiniburn_overworld\","
        "    \"logical_height\": 384,"
        "    \"min_y\": -64,"
        "    \"monster_spawn_block_light_limit\": 0,"
        "    \"monster_spawn_light_level\": {"
        "        \"type\": \"minecraft:uniform\","
        "        \"max_inclusive\": 7,"
        "        \"min_inclusive\": 0"
        "    },"
        "    \"natural\": true,"
        "    \"piglin_safe\": false,"
        "    \"respawn_anchor_works\": false,"
        "    \"ultrawarm\": false"
        "}"
        ));

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "cat_variant");
        pb_write_varint(pb, 1);
        pb_write_string_c(pb, "white");
        pb_write_bool(pb, true);

        pb_nbt_from_json(pb, cJSON_Parse(
            "{"
                "\"asset_id\": \"cat/white\","
                "\"spawn_conditions\": { \"priority\": 0 }"
            "}"
        ));

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "chicken_variant");
        pb_write_varint(pb, 1);
        pb_write_string_c(pb, "temperate");
        pb_write_bool(pb, true);

        pb_nbt_from_json(pb, cJSON_Parse(
            "{" 
                "\"asset_id\": \"chicken/temperate_chicken\","
                "\"spawn_conditions\": { \"priority\": 0 }"
            "}"
        ));

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "cow_variant");
        pb_write_varint(pb, 1);
        pb_write_string_c(pb, "temperate");
        pb_write_bool(pb, true);

        pb_nbt_from_json(pb, cJSON_Parse(
            "{" 
                "\"asset_id\": \"cow/temperate_cow\","
                "\"spawn_conditions\": { \"priority\": 0 }"
            "}"
        ));

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "frog_variant");
        pb_write_varint(pb, 1);
        pb_write_string_c(pb, "temperate");
        pb_write_bool(pb, true);

        pb_nbt_from_json(pb, cJSON_Parse(
            "{" 
                "\"asset_id\": \"frog/temperate_frog\","
                "\"spawn_conditions\": { \"priority\": 0 }"
            "}"
        ));

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "pig_variant");
        pb_write_varint(pb, 1);
        pb_write_string_c(pb, "temperate");
        pb_write_bool(pb, true);

        pb_nbt_from_json(pb, cJSON_Parse(
            "{" 
                "\"asset_id\": \"pig/temperate_pig\","
                "\"spawn_conditions\": { \"priority\": 0 }"
            "}"
        ));

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "painting_variant");
        pb_write_varint(pb, 1);
        pb_write_string_c(pb, "owlemons");
        pb_write_bool(pb, true);

        pb_nbt_from_json(pb, cJSON_Parse(
            "{" 
                "\"asset_id\": \"owlemons\","
                "\"width\": 3,"
                "\"height\": 3,"
                "\"title\": \"OWL!\","
                "\"author\": \"OWL!\""
            "}"
        ));

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "wolf_variant");
        pb_write_varint(pb, 1);
        pb_write_string_c(pb, "pale");
        pb_write_bool(pb, true);

        // 1.21.5
        pb_nbt_from_json(pb, cJSON_Parse(
            "{" 
                "\"assets\": {"
                    "\"angry\": \"wolf/wolf_angry\","
                    "\"tame\": \"wolf/wolf_tame\","
                    "\"wild\": \"wolf/wolf\""
                "},"
                "\"spawn_conditions\": { \"priority\": 0 }"
            "}"
        ));

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_REGISTRY_DATA);
        pb_write_string_c(pb, "wolf_sound_variant");
        pb_write_varint(pb, 1);
        pb_write_string_c(pb, "classic");
        pb_write_bool(pb, true);

        pb_nbt_from_json(pb, cJSON_Parse(
            "{" 
                "\"ambient_sound\": \"minecraft:entity.wolf.ambient\","
                "\"death_sound\": \"minecraft:entity.wolf.death\","
                "\"growl_sound\": \"minecraft:entity.wolf.growl\","
                "\"hurt_sound\": \"minecraft:entity.wolf.hurt\","
                "\"pant_sound\": \"minecraft:entity.wolf.pant\","
                "\"whine_sound\": \"minecraft:entity.wolf.whine\""
            "}"
        ));

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    {
        pb_write_id(pb, P_UPDATE_TAGS);
        pb_write_varint(pb, 0);

        packet = pb_finalize(pb);
        send_finalized_packet(handle, packet, false);
        pb_reset(pb);
    }

    pb_write_id(pb, P_FINISH_CONFIGURATION);
    packet = pb_finalize(pb);
    send_finalized_packet(handle, packet, false);
    LOG_TRACE("S -> C: finish_configuration");
}

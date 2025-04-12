void connection_read_cb(uv_stream_t* handle,
                        ssize_t nread,
                        const uv_buf_t* buf) {
    // XXX(eli): BAD BAD BAD BAD... assuming the buffer contains the entire
    // packets and that there is exactly 1 packet in it

    Client* client = handle->data;

    if (nread > 0) {
        PacketReader pr = { .buffer = *buf };
        PacketBuilder *pb = &client->pb;
        pb_reset(pb);

        int packet_len = pr_read_varint(&pr);
        int packet_id = pr_read_varint(&pr);

        LOG_TRACE("Received packet { state = %d, id = 0x%02X, len = %d }", client->state, packet_id, packet_len);

        switch (packet_id) {
            case 0x00: {
                if (client->state == HANDSHAKE) { 
                    int proto_ver = pr_read_varint(&pr);
                    String server_addr = pr_read_string(&pr);
                    uint16_t port = pr_read_u16(&pr);
                    int next_state = pr_read_varint(&pr);

                    LOG_TRACE(
                        "intention = { proto_ver = %d, server_addr = %.*s, server_port = %d, next_state = %d }", 
                        proto_ver,
                        server_addr.len,
                        server_addr.data,
                        port,
                        next_state
                    );

                    client->state = next_state;
                } else if (client->state == STATUS) {
                    // LOG_TRACE("status_request = {}", 0);

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
                    send_finalized_packet(handle, packet);

                    cJSON_Delete(response);
                } else if (client->state == LOGIN) {
                    String username = pr_read_string(&pr);
                    UUID uuid = pr_read_uuid(&pr);

                    LOG_TRACE("hello = { username = %.*s, uuid = %llX%llX }", 
                              username.len, username.data, 
                              uuid.inner[0], uuid.inner[1]);

                    pb_write_id(pb, P_LOGIN_FINISHED);
                    pb_write_uuid(pb, uuid);
                    pb_write_string(pb, username);
                    pb_write_varint(pb, 0);

                    uv_buf_t packet = pb_finalize(pb);
                    send_finalized_packet(handle, packet);
                }

                break;
            }
            case 0x01: {
                if (client->state == STATUS) {
                    int64_t ping_val = pr_read_i64(&pr);
                    LOG_TRACE("ping_request = { Timestamp = %lld }", ping_val);

                    pb_write_id(pb, P_PONG_RESPONSE);
                    pb_write_i64(pb, ping_val); // Payload

                    uv_buf_t packet = pb_finalize(pb);
                    send_finalized_packet(handle, packet);

                    uv_shutdown_t shutdown_req;
                    uv_shutdown(&shutdown_req, handle, close_connection_cb);

                    client->state = 0;
                }

                break;
            }
            case 0x03: {
                if (client->state == LOGIN) {
                    client->state = CONFIG;

                    pb_write_id(pb, P_SELECT_KNOWN_PACKS);
                    pb_write_varint(pb, 1);
                    pb_write_cstring(pb, "minecraft:core");
                    pb_write_cstring(pb, "0");
                    pb_write_cstring(pb, "1.21.5");

                    uv_buf_t packet = pb_finalize(pb);
                    send_finalized_packet(handle, packet);
                } else if (client->state == CONFIG) {
                    client->state = PLAY;

                    pb_write_id(pb, P_LOGIN);

                    pb_write_u32(pb, 0);                         // Entity ID
                    pb_write_bool(pb, false);                    // Hardcore?

                    pb_write_varint(pb, 1);                      // Dimension names array
                    pb_write_cstring(pb, "minecraft:overworld"); // ...Dimension name

                    pb_write_varint(pb, 3);                      // Max Players
                    pb_write_varint(pb, 16);                     // View Distance
                    pb_write_varint(pb, 16);                     // Simulation Distance
                    pb_write_bool(pb, false);                    // Reduced Debug?
                    pb_write_bool(pb, true);                     // Respawn Screen?
                    pb_write_bool(pb, false);                    // Limited Crafting?
                    pb_write_varint(pb, 0);                      // Dimension ID
                    pb_write_cstring(pb, "minecraft:overworld"); // Dimension ID
                    pb_write_i64(pb, 0);                         // Hashed seed
                    pb_write_u8(pb, 1);                          // Gamemode
                    pb_write_i8(pb, -1);                         // Previous Gamemode
                    pb_write_bool(pb, false);                    // World Debug?
                    pb_write_bool(pb, false);                    // World Superflat?
                    pb_write_bool(pb, false);                    // Death Location?
                    pb_write_varint(pb, 0);                      // Portal Cooldown
                    pb_write_varint(pb, 0);                      // Sea Level
                    pb_write_bool(pb, false);                    // Secure Chat?

                    uv_buf_t packet = pb_finalize(pb);

                    for (int i = 0; i < packet.len; ++i) {
                        printf("%02X", (unsigned char)packet.base[i]);
                        if (i < packet.len + 1) {
                            printf(" ");
                        }
                    }
                    printf("\n");

                    send_finalized_packet(handle, packet);
                }

                break;
            }
            case 0x07: {
                if (client->state == CONFIG) {
                    // pb_write_id(pb, 0x07);
                    // pb_write_cstring(pb, "minecraft:worldgen/biome");
                    // pb_write_varint(pb, 1);
                    //
                    // #define TAG_END 0
                    // #define TAG_BYTE 1
                    // #define TAG_COMPOUND 10
                    //
                    // pb_write_cstring(pb, "minecraft:plains");
                    // pb_write_bool(pb, true);
                    // pb_write_u8(pb, TAG_COMPOUND);
                    // pb_write_u8(pb, TAG_END);
                    //

                    // uv_buf_t data_packet = pb_finalize(pb);
                    // send_finalized_packet(handle, data_packet);
                    // pb_reset(pb);

                    pb_write_id(pb, 0x03);
                    uv_buf_t finish_config_packet = pb_finalize(pb);
                    send_finalized_packet(handle, finish_config_packet);
                }

                break;
            }
            case 0x42: {
                if (client->state == PLAY) {
                    pb_write_id(pb, 0x42);

                    pb_write_varint(pb, 0); // Teleport ID
                    pb_write_f64(pb, 0); // Pos X
                    pb_write_f64(pb, 0); // Pos Y
                    pb_write_f64(pb, 0); // Pos Z

                    pb_write_f64(pb, 0); // Vel X
                    pb_write_f64(pb, 0); // Vel Y
                    pb_write_f64(pb, 0); // Vel Z

                    pb_write_f32(pb, 0); // Yaw
                    pb_write_f32(pb, 0); // Pitch

                    pb_write_u32(pb, 0); // Teleport Flags

                    uv_buf_t packet = pb_finalize(pb);
                    send_finalized_packet(handle, packet);
                }

                break;
            }
        }
    } else {
        if (nread == UV_EOF) {
            uv_shutdown_t shutdown_req;
            uv_shutdown(&shutdown_req, handle, close_connection_cb);
        } else {
            CHECK_UV(nread);
        }
    }
}


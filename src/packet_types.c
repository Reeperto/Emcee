#include "packet_types.h"
#include "data_types.h"
#include "packet.h"

PACKET_WRITER(P_L_CB_Login_Finished) {
    pb_write_id(pb, P_L_CB_LOGIN_FINISHED);

    pb_write_uuid(pb, p->uuid);
    pb_write_string(pb, p->username);

    if (p->entries.proc) {
        p->entries.proc(pb, p->entries.data);
    } else {
        pb_write_varint(pb, 0);
    }
}

PACKET_WRITER(P_C_CB_Finish_Configuration) {
    pb_write_id(pb, P_C_CB_FINISH_CONFIGURATION);
}

PACKET_WRITER(P_L_CB_Registry_Data) {
    pb_write_id(pb, P_C_CB_REGISTRY_DATA);
    pb_write_string(pb, p->registry_id);

    if (p->entries.proc) {
        p->entries.proc(pb, p->entries.data);
    } else {
        pb_write_varint(pb, 0);
    }
}

PACKET_WRITER(P_C_CB_Select_Known_Packs) {
    pb_write_id(pb, P_C_CB_SELECT_KNOWN_PACKS);

    pb_write_varint(pb, p->entries_count);
    
    for (int i = 0; i < p->entries_count; ++i) {
        pb_write_string(pb, p->entries[i].namespace);
        pb_write_string(pb, p->entries[i].identifier);
        pb_write_string(pb, p->entries[i].version);
    }
}

PACKET_WRITER(P_P_CB_Add_Entity) {
    pb_write_id(pb, P_P_CB_ADD_ENTITY);

    pb_write_varint(pb, p->eid);
    pb_write_uuid(pb, p->uuid);
    pb_write_varint(pb, p->entity_type);

    pb_write_f64(pb, p->x);
    pb_write_f64(pb, p->y);
    pb_write_f64(pb, p->z);

    pb_write_i8(pb, p->pitch);
    pb_write_i8(pb, p->yaw);
    pb_write_i8(pb, p->head_yaw);

    pb_write_varint(pb, p->data);

    pb_write_i16(pb, p->v_x);
    pb_write_i16(pb, p->v_y);
    pb_write_i16(pb, p->v_z);
}

PACKET_WRITER(P_P_CB_Block_Changed_Ack) {
    pb_write_id(pb, P_P_CB_BLOCK_CHANGED_ACK);
    pb_write_varint(pb, p->sequence_id);
}

PACKET_WRITER(P_P_CB_Commands) {
    pb_write_id(pb, P_P_CB_COMMANDS);
    pb_write_varint(pb, p->node_count);

    for (int i = 0; i < p->node_count; ++i) {
        CommandNode* node = &p->nodes[i];
        
        u8 flags = node->type;

        flags |= (node->executable << 2);
        flags |= (node->redirect.present << 3);
        flags |= (node->suggestion.present << 4);

        pb_write_u8(pb, flags);

        pb_write_varint(pb, node->children.count);
        for (int j = 0; j < node->children.count; ++j) {
            pb_write_varint(pb, node->children.elems[j]);
        }

        switch (node->type) {
            case CN_ROOT:
                break;
            case CN_LITERAL:
                pb_write_string(pb, node->data.literal.name);
                break;
            case CN_ARGUMENT:
                pb_write_string(pb, node->data.argument.name);
                pb_write_varint(pb, node->data.argument.parser_id);
                break;
        }

        if (node->suggestion.present) {
            pb_write_string(pb, node->suggestion.type);
        }
    }

    pb_write_varint(pb, p->root_idx);
}

PACKET_WRITER(P_P_CB_Login) {
    pb_write_id(pb, P_P_CB_LOGIN);

    pb_write_u32(pb, p->eid);
    pb_write_bool(pb, p->hardcore);

    if (p->dimension_names.elems && p->dimension_names.count != 0) {
        pb_write_varint(pb, p->dimension_names.count);

        for (int i = 0; i < p->dimension_names.count; ++i) {
            pb_write_string(pb, p->dimension_names.elems[i]);
        }
    } else {
        pb_write_varint(pb, 0);
    }

    pb_write_varint(pb, p->max_players);
    pb_write_varint(pb, p->view_distance);
    pb_write_varint(pb, p->simulation_distance);
    pb_write_bool(pb, p->reduced_debug);
    pb_write_bool(pb, p->respawn_screen);
    pb_write_bool(pb, p->limited_crafting);
    pb_write_varint(pb, p->dimension_id);
    pb_write_string(pb, p->dimension_name); 
    pb_write_i64(pb, p->hashed_seed);
    pb_write_u8(pb, p->gamemode);
    pb_write_i8(pb, p->prev_gamemode);
    pb_write_bool(pb, p->debug_mode);
    pb_write_bool(pb, p->superflat);
    pb_write_bool(pb, p->death_location.present);

    if (p->death_location.present) {
        pb_write_string(pb, p->death_location.death_dimension);
        pb_write_position(pb, p->death_location.death_location);
    }

    pb_write_varint(pb, p->portal_cooldown);
    pb_write_varint(pb, p->sea_level);
    pb_write_bool(pb, p->enforce_secure_chat);
}

PACKET_WRITER(P_P_CB_Player_Info_Update) {
    pb_write_id(pb, P_P_CB_PLAYER_INFO_UPDATE);

    pb_write_u8(pb, p->action_set);

    if (p->players.proc) {
        p->players.proc(pb, p->players.data);
    } else {
        pb_write_varint(pb, 0);
    }
}

PACKET_WRITER(P_P_CB_Player_Position) {
    pb_write_id(pb, P_P_CB_PLAYER_POSITION);

    pb_write_varint(pb, p->teleport_id);

    pb_write_f64(pb, p->x);
    pb_write_f64(pb, p->y);
    pb_write_f64(pb, p->z);

    pb_write_f64(pb, p->v_x);
    pb_write_f64(pb, p->v_y);
    pb_write_f64(pb, p->v_z);

    pb_write_f32(pb, p->yaw);
    pb_write_f32(pb, p->pitch);

    pb_write_copy(pb, p->flags.bits, 4);
}

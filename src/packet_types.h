#pragma once

#include <stdbool.h>

#include "data_types.h"
#include "packet.h"
#include "registry_data.h"
#include "util.h"

// TODO(eli): This abstraction is so-so. It avoids type explosion but I want to
// find something better
typedef void (*ArrayFillerProc)(PacketBuilder* pb, void* data);
typedef struct {
    ArrayFillerProc proc;
    void* data;
} ArrayFiller;

#define PACKET_WRITER(PACKET_NAME) \
    void PACKET_NAME##_write(PacketBuilder* pb, PACKET_NAME* p)

// =============================================================================
// HANDSHAKE
// =============================================================================

#define P_H_SB_INTENTION               0x00

// =============================================================================
// STATUS
// =============================================================================

#define P_S_SB_STATUS_REQUEST          0x00
#define P_S_SB_PING_REQUEST            0x01

// -----------------------------------------------------------------------------

#define P_S_CB_STATUS_RESPONSE 0x00

typedef struct {
    JOBJ json_response;
} P_S_CB_Status_Response;

PACKET_WRITER(P_S_CB_Status_Response);

// -----------------------------------------------------------------------------

#define P_S_CB_PONG_RESPONSE 0x01

typedef struct {
    i64 timestamp;
} P_S_CB_Pong_Response;

PACKET_WRITER(P_S_CB_Pong_Response);

// =============================================================================
// LOGIN
// =============================================================================

#define P_L_SB_HELLO 0x00
#define P_L_SB_LOGIN_ACKNOWLEDGED 0x03

// -----------------------------------------------------------------------------

#define P_L_CB_LOGIN_FINISHED 0x02

typedef struct {
    UUID uuid;
    String username;
    ArrayFiller entries;
} P_L_CB_Login_Finished;

PACKET_WRITER(P_L_CB_Login_Finished);

// =============================================================================
// CONFIG
// =============================================================================

#define P_C_SB_FINISH_CONFIGURATION 0x03
#define P_C_SB_SELECT_KNOWN_PACKS 0x07

// -----------------------------------------------------------------------------

#define P_C_CB_FINISH_CONFIGURATION 0x03

typedef struct {} P_C_CB_Finish_Configuration;

PACKET_WRITER(P_C_CB_Finish_Configuration);

// -----------------------------------------------------------------------------

#define P_C_CB_REGISTRY_DATA 0x07

typedef struct {
    String registry_id;
    ArrayFiller entries;
} P_L_CB_Registry_Data;

PACKET_WRITER(P_L_CB_Registry_Data);

// -----------------------------------------------------------------------------

#define P_C_CB_UPDATE_TAGS 0x0D

// -----------------------------------------------------------------------------

#define P_C_CB_SELECT_KNOWN_PACKS 0x0E

typedef struct {
    int entries_count;
    struct {
        String namespace;
        String identifier;
        String version;
    } * entries;
} P_C_CB_Select_Known_Packs;

PACKET_WRITER(P_C_CB_Select_Known_Packs);

// =============================================================================
// PLAY
// =============================================================================

#define P_P_SB_CHAT_COMMAND 0x05
#define P_P_SB_CHAT_COMMAND_SIGNED 0x06
#define P_P_SB_MOVE_PLAYER_POS 0x1C
#define P_P_SB_MOVE_PLAYER_POS_ROT 0x1D
#define P_P_SB_MOVE_PLAYER_ROT 0x1E
#define P_P_SB_PLAYER_ACTION 0x27

// -----------------------------------------------------------------------------

#define P_P_CB_ADD_ENTITY 0x01

typedef struct {
    int eid;
    UUID uuid;
    EntityType entity_type;

    f64 x, y, z;
    i8 pitch, yaw, head_yaw;
    int data;

    i16 v_x, v_y, v_z;
} P_P_CB_Add_Entity;

PACKET_WRITER(P_P_CB_Add_Entity);

// -----------------------------------------------------------------------------

#define P_P_CB_BLOCK_CHANGED_ACK 0x04

typedef struct {
    int sequence_id;
} P_P_CB_Block_Changed_Ack;

PACKET_WRITER(P_P_CB_Block_Changed_Ack);

// -----------------------------------------------------------------------------

#define P_P_CB_COMMANDS 0x10

// TODO(eli): This is a very, very ugly thing. Maybe make this type a little 
// nicer rather than exposing this "raw" description
typedef enum {
    CN_ROOT,
    CN_LITERAL,
    CN_ARGUMENT
} CommandNodeType;

typedef struct {
    CommandNodeType type;

    struct {
        int count;
        int* elems;
    } children;

    bool executable;
    
    struct {
        bool present;
        int node;
    } redirect;

    union {
        struct {} root;

        struct { 
            String name;
        } literal;

        struct {
            String name;
            int parser_id;
            // TODO(eli): Add parser properties somehow
        } argument;
    } data;

    struct {
        bool present;
        String type;
    } suggestion;
} CommandNode;

typedef struct {
    int node_count;
    CommandNode* nodes;
    int root_idx;
} P_P_CB_Commands;

PACKET_WRITER(P_P_CB_Commands);

// -----------------------------------------------------------------------------

#define P_P_CB_GAME_EVENT 0x22

// -----------------------------------------------------------------------------

#define P_P_CB_KEEP_ALIVE 0x26

// -----------------------------------------------------------------------------

#define P_P_CB_LEVEL_CHUNK_WITH_LIGHT 0x27

// -----------------------------------------------------------------------------

#define P_P_CB_LOGIN 0x2B

typedef struct {
    u32 eid;
    bool hardcore;
    struct {
        int count;
        String* elems;
    } dimension_names;
    int max_players;
    int view_distance;
    int simulation_distance;
    bool reduced_debug;
    bool respawn_screen;
    bool limited_crafting;
    int dimension_id;
    String dimension_name;
    i64 hashed_seed;
    u8 gamemode;
    i8 prev_gamemode;
    bool debug_mode;
    bool superflat;

    struct {
        bool present;
        String death_dimension;
        Position death_location;
    } death_location;

    int portal_cooldown;
    int sea_level;
    bool enforce_secure_chat;
} P_P_CB_Login;

PACKET_WRITER(P_P_CB_Login);

// -----------------------------------------------------------------------------

#define P_P_CB_OPEN_SCREEN 0x34

// -----------------------------------------------------------------------------

#define P_P_CB_PLAYER_CHAT 0x3A

typedef struct {
    struct {
        UUID sender;
        int index;
    } header;
} P_P_CB_Player_Chat;

// -----------------------------------------------------------------------------

#define P_P_CB_PLAYER_INFO_UPDATE 0x3F

typedef enum {
    PA_NONE                = 0,
    PA_ADD_PLAYER          = 1 << 0,
    PA_INITIALIZE_CHAT     = 1 << 1,
    PA_UPDATE_GAME_MODE    = 1 << 2,
    PA_UPDATE_LISTED       = 1 << 3,
    PA_UPDATE_LATENCY      = 1 << 4,
    PA_UPDATE_DISPLAY_NAME = 1 << 5,
    PA_UPDATE_LIST_ORDER   = 1 << 6,
    PA_UPDATE_HAT          = 1 << 7,
} PlayerActions;

typedef struct {
    u8 action_set;
    ArrayFiller players;
} P_P_CB_Player_Info_Update;

PACKET_WRITER(P_P_CB_Player_Info_Update);

// -----------------------------------------------------------------------------

#define P_P_CB_PLAYER_POSITION 0x41

typedef enum {
    RELATIVE_X,
    RELATIVE_Y,
    RELATIVE_Z,
    RELATIVE_YAW,
    RELATIVE_PITCH,
    RELATIVE_VEL_X,
    RELATIVE_VEL_Y,
    RELATIVE_VEL_Z,
    PRE_ROTATE_VEL,
    TELEPORT_FLAG_COUNT
} TeleportFlag;

typedef struct {
    u8 bits[4];
} TeleportFlags;

typedef struct {
    int teleport_id;
    f64 x, y, z;
    f64 v_x, v_y, v_z;
    f32 yaw, pitch;
    TeleportFlags flags;
} P_P_CB_Player_Position;

PACKET_WRITER(P_P_CB_Player_Position);

// -----------------------------------------------------------------------------

#define P_P_CB_SET_CHUNK_CACHE_CENTER 0x57

// -----------------------------------------------------------------------------

#define P_P_CB_SET_ENTITY_DATA 0x5C

// -----------------------------------------------------------------------------

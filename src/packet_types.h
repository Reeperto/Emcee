#pragma once

#include <stdbool.h>

// HANDSHAKE
#define P_H_SB_INTENTION               0x00

// STATUS
#define P_S_SB_STATUS_REQUEST          0x00
#define P_S_SB_PING_REQUEST            0x01

#define P_S_CB_STATUS_RESPONSE         0x00
#define P_S_CB_PONG_RESPONSE           0x01

// LOGIN
#define P_L_SB_HELLO                   0x00
#define P_L_SB_LOGIN_ACKNOWLEDGED      0x03

#define P_L_CB_LOGIN_FINISHED          0x02

// CONFIG
#define P_C_SB_FINISH_CONFIGURATION    0x03
#define P_C_SB_SELECT_KNOWN_PACKS      0x07

#define P_C_CB_FINISH_CONFIGURATION    0x03
#define P_C_CB_REGISTRY_DATA           0x07
#define P_C_CB_UPDATE_TAGS             0x0D
#define P_C_CB_SELECT_KNOWN_PACKS      0x0E

// PLAY
#define P_P_SB_CHAT_COMMAND            0x05

#define P_P_CB_ADD_ENTITY              0x01
#define P_P_CB_GAME_EVENT              0x22
#define P_P_CB_KEEP_ALIVE              0x26
#define P_P_CB_LEVEL_CHUNK_WITH_LIGHT  0x27
#define P_P_CB_LOGIN                   0x2B
#define P_P_CB_OPEN_SCREEN             0x34
#define P_P_CB_PLAYER_INFO_UPDATE      0x41
#define P_P_CB_PLAYER_POSITION         0x41
#define P_P_CB_SET_CHUNK_CACHE_CENTER  0x57
#define P_P_CB_SET_ENTITY_DATA         0x5C

typedef enum {
    ADD_PLAYER,
    INITIALIZE_CHAT,
    UPDATE_GAME_MODE,
    UPDATE_LISTED,
    UPDATE_LATENCY,
    UPDATE_DISPLAY_NAME,
    UPDATE_LIST_ORDER,
    UPDATE_HAT
} PlayerActions;

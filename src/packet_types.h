#pragma once

#include <stdbool.h>

#define P_HELLO                  0x00
#define P_INTENTION              0x00
#define P_PING_REQUEST           0x00
#define P_PONG_RESPONSE          0x00
#define P_STATUS_REQUEST         0x00
#define P_STATUS_RESPONSE        0x00

#define P_LOGIN_FINISHED         0x02

#define P_FINISH_CONFIGURATION   0x03

#define P_REGISTRY_DATA          0x07

#define P_UPDATE_TAGS            0x0D

#define P_SELECT_KNOWN_PACKS     0x0E

#define P_GAME_EVENT             0x22

#define P_KEEP_ALIVE             0x26

#define P_LEVEL_CHUNK_WITH_LIGHT 0x27

#define P_LOGIN                  0x2B

#define P_PLAYER_POSITION        0x41

#define P_SET_CHUNK_CACHE_CENTER 0x57

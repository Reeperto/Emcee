#pragma once

#include <stdlib.h>

static const char* worldgen_biome_entries[] = {
    "badlands",
    "bamboo_jungle",
    "basalt_deltas",
    "beach",
    "birch_forest",
    "cherry_grove",
    "cold_ocean",
    "crimson_forest",
    "dark_forest",
    "deep_cold_ocean",
    "deep_dark",
    "deep_frozen_ocean",
    "deep_lukewarm_ocean",
    "deep_ocean",
    "desert",
    "dripstone_caves",
    "end_barrens",
    "end_highlands",
    "end_midlands",
    "eroded_badlands",
    "flower_forest",
    "forest",
    "frozen_ocean",
    "frozen_peaks",
    "frozen_river",
    "grove",
    "ice_spikes",
    "jagged_peaks",
    "jungle",
    "lukewarm_ocean",
    "lush_caves",
    "mangrove_swamp",
    "meadow",
    "mushroom_fields",
    "nether_wastes",
    "ocean",
    "old_growth_birch_forest",
    "old_growth_pine_taiga",
    "old_growth_spruce_taiga",
    "pale_garden",
    "plains",
    "river",
    "savanna",
    "savanna_plateau",
    "small_end_islands",
    "snowy_beach",
    "snowy_plains",
    "snowy_slopes",
    "snowy_taiga",
    "soul_sand_valley",
    "sparse_jungle",
    "stony_peaks",
    "stony_shore",
    "sunflower_plains",
    "swamp",
    "taiga",
    "the_end",
    "the_void",
    "warm_ocean",
    "warped_forest",
    "windswept_forest",
    "windswept_gravelly_hills",
    "windswept_hills",
    "windswept_savanna",
    "wooded_badlands",
    NULL
};

static const char* damage_type_entries[] = {
    "arrow",
    "bad_respawn_point",
    "cactus",
    "campfire",
    "cramming",
    "dragon_breath",
    "drown",
    "dry_out",
    "ender_pearl",
    "explosion",
    "fall",
    "falling_anvil",
    "falling_block",
    "falling_stalactite",
    "fireball",
    "fireworks",
    "fly_into_wall",
    "freeze",
    "generic",
    "generic_kill",
    "hot_floor",
    "in_fire",
    "in_wall",
    "indirect_magic",
    "lava",
    "lightning_bolt",
    "mace_smash",
    "magic",
    "mob_attack",
    "mob_attack_no_aggro",
    "mob_projectile",
    "on_fire",
    "out_of_world",
    "outside_border",
    "player_attack",
    "player_explosion",
    "sonic_boom",
    "spit",
    "stalagmite",
    "starve",
    "sting",
    "sweet_berry_bush",
    "thorns",
    "thrown",
    "trident",
    "unattributed_fireball",
    "wind_charge",
    "wither",
    "wither_skull",
    NULL
};

static const char* chat_type_entries[] = {
    "chat",
    "emote_command",
    "msg_command_incoming",
    "msg_command_outgoing",
    "say_command",
    "team_msg_command_incoming",
    "team_msg_command_outgoing",
    NULL
};

static const char* dimension_type_entries[] = {
    "overworld",
    "overworld_caves",
    "the_end",
    "the_nether",
    NULL
};

static const char* cat_variant_entries[] = {
    "all_black",
    "black",
    "british_shorthair",
    "calico",
    "jellie",
    "persian",
    "ragdoll",
    "red",
    "siamese",
    "tabby",
    "white",
    NULL
};

static const char* chicken_variant_entries[] = {
    "cold",
    "temperate",
    "warm",
    NULL
};

static const char* cow_variant_entries[] = {
    "cold",
    "temperate",
    "warm",
    NULL
};

static const char* frog_variant_entries[] = {
    "cold",
    "temperate",
    "warm",
    NULL
};

static const char* painting_variant_entries[] = {
    "alban",
    "aztec",
    "aztec2",
    "backyard",
    "baroque",
    "bomb",
    "bouquet",
    "burning_skull",
    "bust",
    "cavebird",
    "changing",
    "cotan",
    "courbet",
    "creebet",
    "donkey_kong",
    "earth",
    "endboss",
    "fern",
    "fighters",
    "finding",
    "fire",
    "graham",
    "humble",
    "kebab",
    "lowmist",
    "match",
    "meditative",
    "orb",
    "owlemons",
    "passage",
    "pigscene",
    "plant",
    "pointer",
    "pond",
    "pool",
    "prairie_ride",
    "sea",
    "skeleton",
    "skull_and_roses",
    "stage",
    "sunflowers",
    "sunset",
    "tides",
    "unpacked",
    "void",
    "wanderer",
    "wasteland",
    "water",
    "wind",
    "wither",
    NULL
};

static const char* pig_variant_entries[] = {
    "cold",
    "temperate",
    "warm",
    NULL
};

static const char* wolf_sound_variant_entries[] = {
    "angry",
    "big",
    "classic",
    "cute",
    "grumpy",
    "puglin",
    "sad",
    NULL
};

static const char* wolf_variant_entries[] = {
    "ashen",
    "black",
    "chestnut",
    "pale",
    "rusty",
    "snowy",
    "spotted",
    "striped",
    "woods",
    NULL
};

struct RegistryPair {
    const char* registry_name;
    const char** entries;
};

static struct RegistryPair registries[] = {
    { "minecraft:worldgen/biome",     worldgen_biome_entries    },
    { "minecraft:damage_type",        damage_type_entries       },
    { "minecraft:chat_type",          chat_type_entries         },
    { "minecraft:dimension_type",     dimension_type_entries    },
    { "minecraft:cat_variant",        cat_variant_entries       },
    { "minecraft:chicken_variant",    chicken_variant_entries   },
    { "minecraft:cow_variant",        cow_variant_entries       },
    { "minecraft:frog_variant",       frog_variant_entries      },
    { "minecraft:painting_variant",   painting_variant_entries  },
    { "minecraft:pig_variant",        pig_variant_entries       },
    { "minecraft:wolf_sound_variant", wolf_sound_variant_entries},
    { "minecraft:wolf_variant",       wolf_variant_entries      },
    { NULL,                           NULL                      }
};

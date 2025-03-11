#pragma once

enum {

    NUM_RESOURCES,
};

#define ES_LOCKED               (1ULL << 0)  /* saved entity is locked */
#define ES_MOVEABLE             (1ULL << 1)
#define ES_AXIS_ROT             (1ULL << 2)
#define ES_SLEEPING             (1ULL << 3)
#define ES_DISABLE_UNLOADING    (1ULL << 4)

#define NUM_LAYERS 3
#define LAYER_DEPTH 1.f

#define TARGET_DIST_SCALE .75f

enum {
    ENTITY_EVENT_REMOVE,
    ENTITY_EVENT_DEATH,

    ENTITY_EVENT__NUM
};

/* id types */
#define LEVEL_LOCAL   0
#define LEVEL_DB      1
#define LEVEL_MAIN    2
#define LEVEL_SYS     3
#define LEVEL_PARTIAL 4

enum {
    SND_WOOD_METAL,
    SND_WOOD_WOOD,
    SND_WOOD_HOLLOWWOOD,
    SND_CLICK,
    SND_ROCKET,
    SND_EXPLOSION,
    SND_EXPLOSION_LIGHT,
    SND_SHEET_METAL,
    SND_RUBBER,
    SND_METAL_METAL,
    SND_METAL_METAL2,
    SND_STONE_STONE,
    SND_WIN,

    SND__NUM
};

#pragma once

#include "ud2.hh"

enum {
    TPIXEL_MATERIAL_GRASS,      // 0
    TPIXEL_MATERIAL_DIRT,       // 1
    TPIXEL_MATERIAL_HEAVY_DIRT, // 2
    TPIXEL_MATERIAL_ROCK,       // 3

    TPIXEL_MATERIAL_DIAMOND_ORE,
    TPIXEL_MATERIAL_RUBY_ORE,
    TPIXEL_MATERIAL_SAPPHIRE_ORE,
    TPIXEL_MATERIAL_EMERALD_ORE,
    TPIXEL_MATERIAL_TOPAZ_ORE,
    TPIXEL_MATERIAL_COPPER_ORE,
    TPIXEL_MATERIAL_IRON_ORE,
    TPIXEL_MATERIAL_ALUMINIUM_ORE,

    NUM_TPIXEL_MATERIALS,
};

struct tpixel_desc_1_5 : public ud2_info {
    /* static data */
    uint8_t size;
    uint8_t pos;
    uint8_t material;
    uint8_t r;

    /* runtime data */
    float oil;
    float hp;
    uint8_t grass;
};

struct tpixel_desc : public ud2_info {
    /* static data */
    uint8_t size;
    uint8_t pos;
    uint8_t material;
    uint8_t r;

    /* runtime data */
    float oil;
    float hp;
    uint8_t grass;


    struct tpixel_desc_1_5_1 {
        uint8_t half; /* 0 = full pixel, 1..4 = corner */
    } desc_1_5_1;

    tpixel_desc() : ud2_info(UD2_TPIXEL_DESC) { desc_1_5_1.half = 0; }

    inline int get_local_x(){return pos & 15;}
    inline int get_local_y(){return pos >> 4;}
    inline void reset()
    {
        switch (this->material) {
            case TPIXEL_MATERIAL_DIRT:
            case TPIXEL_MATERIAL_GRASS:
                this->hp = 2.f;
                break;

            case TPIXEL_MATERIAL_HEAVY_DIRT:
                this->hp = 4.f;
                break;

            case TPIXEL_MATERIAL_ROCK:
                this->hp = 6.f;
                break;

            default:
                this->hp = 12.f;
                break;
        }

        this->oil = 100.f * (material*3) * (size*3);
        this->grass = 0;
        this->desc_1_5_1.half = 0;
    }
};

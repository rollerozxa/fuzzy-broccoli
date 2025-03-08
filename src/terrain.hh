#pragma once

#include <map>
#include <stdint.h>
#include "types.hh"
#include <cmath>

struct terrain_coord {
    int chunk_x;
    int chunk_y;
    uint8_t _xy;

    terrain_coord(){};

    terrain_coord(float world_x, float world_y)
    {
        this->set_from_world(world_x, world_y);
    }

    inline int get_global_x()
    {
        return chunk_x*16+this->get_local_x();
    }

    inline int get_global_y()
    {
        return chunk_y*16+this->get_local_y();
    }

    inline float get_world_x()
    {
        return this->chunk_x * 8.f + this->get_local_x()*.5f;
    }

    inline float get_world_y()
    {
        return this->chunk_y * 8.f + this->get_local_y()*.5f;
    }

    inline void set_from_world(float x, float y)
    {
        int gx = (int)roundf(x * 2.f);
        int gy = (int)roundf(y * 2.f);
        this->set_from_global(gx, gy);
    }

    inline void set_from_global(int gx, int gy)
    {
        this->chunk_x = (int)floor(gx/16.f);
        this->chunk_y = (int)floor(gy/16.f);

        int local_x = (int)(gx-this->chunk_x*16) & 0x0f;
        int local_y = (int)(gy-this->chunk_y*16) & 0x0f;

        this->_xy = (local_y << 4) | local_x;
    }

    void step(int x, int y)
    {
        int gx = this->chunk_x * 16 + this->get_local_x();
        int gy = this->chunk_y * 16 + this->get_local_y();

        gx += x;
        gy += y;

        this->set_from_global(gx, gy);
    }

    chunk_pos get_chunk_pos()
    {
        return chunk_pos(chunk_x, chunk_y);
    }

    int get_local_x()
    {
        return (_xy) & 0xf;
    }

    int get_local_y()
    {
        return (_xy >> 4) & 0xf;
    }
};

/**
 * Comparison of terrain_coords are done by comparing the chunk positions,
 * this allows us to easily sort terrain_coords by chunk and quickly
 * apply a transaction
 * */
bool operator <(const terrain_coord& lhs, const terrain_coord &rhs);

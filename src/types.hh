#pragma once

#include <stdint.h>

class entity;

struct chunk_pos {
    int x, y;

    chunk_pos(int x, int y) {
        this->x = x;
        this->y = y;
    };
};

struct chunk_intersection {
    int x;
    int y;
    int num_fixtures;
};

bool operator <(const chunk_pos& lhs, const chunk_pos &rhs);

typedef ::entity p_entity;
typedef uint32_t p_id;
typedef uint8_t p_gid;

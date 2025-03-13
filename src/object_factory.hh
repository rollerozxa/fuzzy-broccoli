#pragma once

#include <stdint.h>
#include <Box2D/Common/b2Math.h>
#include "types.hh"
#include <vector>

enum {
    O_PLANK                 = 0,
    O_BALL                  = 1,
    O_CYLINDER              = 2,
    O_PLATFORM              = 3,
    O_WHEEL                 = 4,
    O_DAMPER                = 5,
    O_RC_BASIC              = 6,
    O_POWER_SUPPLY          = 7,
    O_POWER_CABLE           = 8,
    O_SIGNAL_CABLE          = 9,
    O_BUTTON                = 10,
    O_DAMPER_2              = 11,
    O_ROCKET                = 12,
    O_YSPLITTER             = 13,
    O_INTERACTIVE_CYLINDER  = 14,
    O_LAND_MINE             = 15,
    O_RUBBERBAND            = 16,
    O_RUBBERBAND_2          = 17,
    O_SIMPLE_MOTOR          = 18,
    O_WEIGHT                = 19,

    MAX_OF_ID,
};

class lvlbuf;
class entity;
class group;

class of
{
  public:
    static uint32_t _id; /* global entity id counter */

    static const int num_categories = 1;

    static int get_num_objects(int cat);
    static const char *get_category_name(int x);
    static const char *get_category_hint(int x);

    static entity *create(p_gid g_id);
    static entity *create_with_id(p_gid g_id, uint32_t id);

    static void init(void);

    static entity* read(lvlbuf *lb, uint8_t version, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f), std::vector<chunk_pos> *affected_chunks=0);
    static void write(lvlbuf *lb, uint8_t version, entity *e, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f), bool write_states=false);

    static group* read_group(lvlbuf *lb, uint8_t version, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f));
    static void write_group(lvlbuf *lb, uint8_t version, group *e, uint32_t id_modifier=0, b2Vec2 displacement=b2Vec2(0.f,0.f), bool write_states=false);

    static int get_gid(int category, int child);
    static uint32_t get_next_id(void);
};

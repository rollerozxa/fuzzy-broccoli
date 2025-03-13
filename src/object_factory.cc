#include "object_factory.hh"
#include "ball.hh"
#include "beam.hh"
#include "button.hh"
#include "cable.hh"
#include "cylinder.hh"
#include "damper.hh"
#include "explosive.hh"
#include "generator.hh"
#include "group.hh"
#include "motor.hh"
#include "panel.hh"
#include "rocket.hh"
#include "rubberband.hh"
#include "shelf.hh"
#include "weight.hh"
#include "wheel.hh"
#include "ysplitter.hh"

static entity* new_thinplank(void){return new beam(BEAM_THIN);};
static entity* new_shelf(void){return new shelf();};

static entity* new_ball_wood(void){return new ball();};
static entity* new_generator(void){return new generator();};
static entity* new_powercable(void){return new cable(CABLE_BLACK);};
static entity* new_signalcable(void){return new cable(CABLE_RED);};
static entity* new_button(void){return new button(0);};

static entity* new_wheel(void){return new wheel();};
static entity* new_cylinder(void){return new cylinder(0);};
static entity* new_smallpanel(void){return new panel();};
static entity* new_simplemotor(void){return new motor();};

static entity* new_damper1(void){return new damper_1();};
static entity* new_damper2(void){return new damper_2();};
static entity* new_rocket(void){return new rocket();};
static entity* new_ysplitter(void){return new ysplitter();};
static entity* new_interactive_cylinder(void){return new cylinder(1);};

static entity* new_landmine(void){return new explosive();};
static entity* new_rubberband1(void){return new rubberband_1();};
static entity* new_rubberband2(void){return new rubberband_2();};
static entity* new_weight(void){return new weight();};

uint32_t of::_id = 1;

static entity* (*c_creator[])(void) = {
    &new_thinplank,
    &new_ball_wood,
    &new_cylinder,
    &new_shelf,
    &new_wheel,
    &new_damper1,
    &new_smallpanel,
    &new_generator,
    &new_powercable,
    &new_signalcable,
    &new_button,
    &new_damper2,
    &new_rocket,
    &new_ysplitter,
    &new_interactive_cylinder,
    &new_landmine,
    &new_rubberband1,
    &new_rubberband2,
    &new_simplemotor,
    &new_weight,
};

static int num_creators = sizeof(c_creator)/sizeof(void*);

static const char *categories[] = {
 "Cat",
};

static const char *category_hints[] = {
 "bas",
};

static int c0_ids[] = {
    O_CYLINDER,
    O_PLANK,
    O_RUBBERBAND,
    O_WEIGHT,
    O_PLATFORM,
    O_WHEEL,
    O_RC_BASIC,
    O_SIGNAL_CABLE,
    O_POWER_CABLE,
    O_POWER_SUPPLY,
    O_SIMPLE_MOTOR,
    O_INTERACTIVE_CYLINDER,
    // Marble goal
    O_BALL,
    O_DAMPER,
    O_BUTTON,
    O_ROCKET,
    O_YSPLITTER,
    O_LAND_MINE,
};

static const int num_objects[of::num_categories] = {
    (sizeof(c0_ids)/sizeof(int)),
};

static int *ids[] = {
    c0_ids
};

void
of::init(void)
{

}

int of::get_gid(int category, int child)
{
    /* TODO: bounds check */
    return ids[category][child];
}

int of::get_num_objects(int cat)
{
    return num_objects[cat];
}

const char *
of::get_category_name(int x)
{
    return categories[x];
}

const char *
of::get_category_hint(int x)
{
    return category_hints[x];
}

static entity*
_create(p_gid id)
{
    entity *e = 0;

    if (id < num_creators)
        e = ((*c_creator[id]))();

    if (e) {
        e->g_id = id;
    }

    return e;
}

entity*
of::create(p_gid g_id)
{
    entity *e = _create(g_id);

    if (e) {
        e->id = of::get_next_id();
    }

    return e;
}

entity*
of::create_with_id(p_gid g_id, p_id id)
{
    entity *e = _create(g_id);

    if (e) {
        e->id = id;
    }

    return e;
}

/* IMPORTANT: keep this in sync with
 * lvledit::print_gids()
 * and
 * chunk_preloader::preload_entity() */
entity*
of::read(lvlbuf *lb, uint8_t version, uint32_t id_modifier, b2Vec2 displacement, std::vector<chunk_pos> *affected_chunks)
{
    entity *e;
    p_gid g_id;
    uint8_t np, nc;
    uint32_t group_id;
    uint32_t id;

    /* XXX GID XXX */
    g_id = lb->r_uint8();
    id = lb->r_uint32() + id_modifier;
    group_id = (uint32_t)lb->r_uint16();
    group_id = group_id | (lb->r_uint16() << 16);

    //tms_debugf("read g_id: %u", g_id);

    if (group_id != 0) group_id += id_modifier;

    e = of::create_with_id(g_id, id);

    if (e) {
        np = lb->r_uint8();

        if (version >= LEVEL_VERSION_1_5) {
            nc = lb->r_uint8();
        } else
            nc = 0;

        e->gr = (group*)(uintptr_t)group_id;

        // XXX: Should this check if id != world->level.adventure_id instead?
        // XXX: No, it should not
        if (id >= of::_id && id != 0xffffffff) of::_id = id+1;

        e->g_id = g_id;

        if (np != e->num_properties) {
            if (np > e->num_properties) {
                tms_errorf("Too many properties for object %d, will try to compensate.", e->g_id);
                //np = e->num_properties;
                //return 0;
            }
        }

        e->_pos.x = lb->r_float();
        e->_pos.y = lb->r_float();

        /* if we're grouped, the pos is local within the group, do not add displacement
         * since displacement has already been added to the group itself */
        if (e->gr == 0) {
            e->_pos.x += displacement.x;
            e->_pos.y += displacement.y;
        }

        e->_angle = lb->r_float();
        e->set_layer((int)lb->r_uint8());

        if (version >= LEVEL_VERSION_1_5) {
            e->load_flags(lb->r_uint64());

            for (int x=0; x<nc; x++) {
                int cx = lb->r_int32(); /* chunk x */
                int cy = lb->r_int32(); /* chunk y */

                if (e->flag_active(ENTITY_STATE_SLEEPING)) {
                    e->chunk_intersections[x].x = cx;
                    e->chunk_intersections[x].y = cy;
                    e->chunk_intersections[x].num_fixtures = 1;
                    e->num_chunk_intersections++;
                }

                /* if we have a pointer to an "affected chunks" vector, insert info there */
                if (affected_chunks) {
                    affected_chunks->push_back(chunk_pos(cx, cy));
                }
            }
        } else {
            e->set_flag(ENTITY_AXIS_ROT, (bool)lb->r_uint8());

            if (version >= 10) {
                e->set_moveable((bool)lb->r_uint8());
            } else {
                e->set_moveable(true);
            }
        }

        for (int x=0; x<np; x++) {
            uint8_t type = lb->r_uint8();

            if (x >= e->num_properties) {
                tms_infof("Skipping property, type %d", type);
                switch (type) {
                    case P_INT8: lb->r_uint8(); break;
                    case P_INT: lb->r_uint32(); break;
                    case P_ID: lb->r_uint32(); break;
                    case P_FLT: lb->r_float(); break;
                    case P_STR:
                        {
                            uint32_t len;
                            if (version >= LEVEL_VERSION_1_5) {
                                len = lb->r_uint32();
                            } else {
                                len = lb->r_uint16();
                            }

                            lb->rp += len;
                        }
                        break;
                }
                continue;
            }

            property *p = &e->properties[x];

            if (type != p->type) {
                if (p->type == P_FLT && type == P_INT8) {
                    uint8_t v = lb->r_uint8();
                    p->v.f = (float)v;

                    tms_infof("Read uint8 %u from file, converted it to float %f", v, p->v.f);
                } else if (p->type == P_FLT && type == P_INT) {
                    uint32_t v = lb->r_uint32();
                    p->v.f = (float)v;

                    tms_infof("Read uint32 %u from file, converted it to float %f", v, p->v.f);
                } else if (p->type == P_INT && type == P_INT8) {
                    uint8_t v = lb->r_uint8();
                    p->v.i = (uint32_t)v;

                    tms_infof("Read uint8 %d from file, converted it to uint32 %d", v, p->v.i);
                } else if (p->type == P_INT8 && type == P_INT) {
                    uint32_t v = lb->r_uint32();
                    p->v.i8 = (uint8_t)v;
                    tms_infof("Read uint32 %d from file, converted it to uint8 %d", v, p->v.i8);
                } else if (type == P_INT8 || p->type == P_INT8 || p->type == P_STR || type == P_STR) {
                    switch (type) {
                        case P_INT8: lb->r_uint8(); break;
                        case P_INT: lb->r_uint32(); break;
                        case P_ID: lb->r_uint32(); break;
                        case P_FLT: lb->r_float(); break;
                        case P_STR:
                                    {
                                        uint32_t len = 0;
                                        if (version >= LEVEL_VERSION_1_5) {
                                            len = lb->r_uint32();
                                        } else {
                                            len = lb->r_uint16();
                                        }
                                        char *buf = (char*)malloc(len);
                                        lb->r_buf(buf, len);
                                        free(buf);
                                    }
                                    break;
                        default: tms_fatalf("invalid object property %d", type);
                    }
                    /* TODO: Should it gracefully quit? This should be an error
                     *       that will sort itself out when the level is re-saved */
                    tms_errorf("incorrect property type when loading properties");
                    memset(&p->v, 0, sizeof(p->v));
                } else if ((p->type == P_INT && type == P_ID)
                        || (p->type == P_ID && type == P_INT)) {
                    p->v.i = lb->r_uint32();
                }
            } else {
                switch (type) {
                    case P_INT8: p->v.i8 = lb->r_uint8(); break;
                    case P_INT: p->v.i = lb->r_uint32(); break;
                    case P_ID: p->v.i = lb->r_uint32() + id_modifier; break;
                    //case P_ID: p->v.i = lb->r_uint32(); break;
                    case P_FLT: p->v.f = lb->r_float(); break;
                    case P_STR:
                        if (version >= LEVEL_VERSION_1_5) {
                            p->v.s.len = lb->r_uint32();
                        } else {
                            p->v.s.len = lb->r_uint16();
                        }
                        p->v.s.buf = (char*)malloc(p->v.s.len+1);
                        lb->r_buf(p->v.s.buf, p->v.s.len);
                        p->v.s.buf[p->v.s.len] = '\0';
                        break;

                    default:
                        tms_fatalf("invalid object property %d", type);
                }
            }
        }
    } else {
        tms_errorf("invalid object: %d", g_id);
    }

    return e;
}

group *
of::read_group(lvlbuf *lb, uint8_t version, uint32_t id_modifier, b2Vec2 displacement)
{
    /* XXX keep in sync with chunk_preload::read_group() */

    group *g = new group();
    g->id = lb->r_uint32() + id_modifier;
    g->_pos.x = lb->r_float() + displacement.x;
    g->_pos.y = lb->r_float() + displacement.y;
    g->_angle = lb->r_float();

    if (g->id >= of::_id) of::_id = g->id+1;

    return g;
}

void
of::write_group(lvlbuf *lb, uint8_t version, group *e, uint32_t id_modifier, b2Vec2 displacement, bool write_states/*=false*/)
{
    lb->ensure(
             4 /* id */
            +4 /* pos x */
            +4 /* pos y */
            +4 /* angle */
            );

    b2Vec2 p = e->get_position();

    e->write_ptr = lb->size;

    lb->w_uint32(e->id + id_modifier);
    lb->w_float(p.x + displacement.x);
    lb->w_float(p.y + displacement.y);
    lb->w_float(e->get_angle());

    e->write_size = lb->size - e->write_ptr;
}

void
of::write(lvlbuf *lb, uint8_t version, entity *e, uint32_t id_modifier, b2Vec2 displacement, bool write_states/*=false*/)
{
    e->write_ptr = lb->size;

    /* XXX GID XXX */
    /* we always ensure the "worst case scenario" */
    lb->ensure(1 /* uint8_t, g_id */
              +4 /* uint32_t, id */
              +2 /* uint16_t, group id low */
              +2 /* uint16_t, group id high */
              +1 /* uint8_t, num properties */
              +1 /* uint8_t, num_chunk_intersections */
              +4 /* float, pos.x */
              +4 /* float, pos.y */
              +4 /* float, angle */
              +1 /* uint8_t, layer */
              +8 /* uint64_t, flags */
              +1 /* some remaining stuff we might as well leave ;-) */
              +1
              +1
              +8
              +4
            );

    /* XXX GID XXX */
    lb->w_uint8(e->g_id);
    lb->w_uint32(e->id + id_modifier);

    uint32_t group_id = (e->gr ? e->gr->id + id_modifier : 0);
    uint16_t group_id_low = (uint16_t)(group_id & 0xffff);
    uint16_t group_id_high = (uint16_t)((group_id >> 16) & 0xffff);
    //tms_infof("write entity %u, group %u", e->id+id_modifier, group_id);

    lb->w_uint16(group_id_low);
    lb->w_uint16(group_id_high); /* unused, was breadboard id */

    lb->w_uint8(e->num_properties);

    if (version >= LEVEL_VERSION_1_5) {
        lb->w_uint8(e->num_chunk_intersections);
    }

    /* only add displacement if we're not in a group,
     * since we have local coordinates if we're in a group */
    lb->w_float(e->_pos.x + (e->gr == 0 ? displacement.x : 0.f));
    lb->w_float(e->_pos.y + (e->gr == 0 ? displacement.y : 0.f));
    lb->w_float(e->_angle);
    lb->w_uint8((uint8_t)e->get_layer());

    if (version >= LEVEL_VERSION_1_5) {
        lb->w_uint64(e->save_flags());

        /* write list of chunks */
        lb->ensure(e->num_chunk_intersections * sizeof(uint32_t) * 2);

        for (int x=0; x<e->num_chunk_intersections; x++) {
#ifdef DEBUG_PRELOADER_SANITY
            tms_assertf(std::abs(e->chunk_intersections[x].x) < 1000 && std::abs(e->chunk_intersections[x].y) < 1000,
                    "suspicious chunk intersection during write");
#endif
            lb->w_int32(e->chunk_intersections[x].x);
            lb->w_int32(e->chunk_intersections[x].y);
        }
    } else {
        lb->w_uint8((uint8_t)e->flag_active(ENTITY_AXIS_ROT));

        if (version >= 10) {
            lb->w_uint8((uint8_t)e->is_moveable());
        }
    }

    for (int x=0; x<e->num_properties; x++) {
        property *p = &e->properties[x];
        lb->w_s_uint8(p->type);

        switch (p->type) {
            case P_INT8: lb->w_s_uint8(p->v.i8); break;
            case P_INT: lb->w_s_uint32(p->v.i); break;
            case P_ID:  lb->w_s_uint32(p->v.i + id_modifier); break;
            //case P_ID:  lb->w_s_uint32(p->v.i); break;
            case P_FLT: lb->w_s_float(p->v.f); break;
            case P_STR:
                if (version >= LEVEL_VERSION_1_5) {
                    lb->w_s_uint32(p->v.s.len);
                } else {
                    lb->w_s_uint16(p->v.s.len);
                }

                lb->w_s_buf(p->v.s.buf, p->v.s.len);
                break;

            default:
                tms_errorf("invalid property type");
                break;
        }
    }

    e->write_size = lb->size - e->write_ptr;
}

uint32_t
of::get_next_id(void)
{
    return of::_id++;
}

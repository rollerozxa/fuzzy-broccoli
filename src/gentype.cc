#include "gentype.hh"
#include "game.hh"
#include "object_factory.hh"
#include "world.hh"

uint32_t _gentype_id = 1;

gentype::~gentype()
{
#ifdef DEBUG
    tms_debugf("clearing those genslots for %p! (loaded:%d, count:%d, sorting:%d)", this, this->loaded, (int)this->genslots.size(), this->sorting);
#endif

#if 0
    {
        /* for easier debugging */
        std::sort(this->genslots.begin(), this->genslots.end());

    }
#endif

    for (std::vector<genslot>::iterator it = this->genslots.begin();
            it != this->genslots.end(); ++it) {
        genslot g = *it;

        level_chunk *c = W->cwindow->get_chunk(g.chunk_x, g.chunk_y, false, false);

#ifdef DEBUG_SPECIFIC_CHUNK
        if (g.chunk_x == DEBUG_CHUNK_X && g.chunk_y == DEBUG_CHUNK_Y) {
            tms_debugf("clearing genslot in %d %d", DEBUG_CHUNK_X, DEBUG_CHUNK_Y);
        }
#endif

        if (c) {
            if (c->genslots[g.slot_x][g.slot_y][g.sorting] == this) {
                tms_assertf(c->generate_phase < (5+g.sorting), "this should never happen!");
                c->genslots[g.slot_x][g.slot_y][g.sorting] = 0;
            }
        }
#ifdef DEBUG
        else {
            tms_warnf("could not find level chunk to remove my genslot!");
        }
#endif
    }

    if (!this->lock) {
        W->cwindow->preloader.gentypes.erase(this->id);
    } else {
        tms_debugf("i'm locked :(");
    }
}


/**
 * Things that can be generated
 **/
struct gentype_generator gentype::gentypes[NUM_GENTYPES] = {

};

bool
gentype::post_occupy()
{
    this->transaction.occupy(this);

    switch (this->transaction.state) {
        case TERRAIN_TRANSACTION_OCCUPIED:
            return !this->genslots.empty();
            break;

        default:
            tms_debugf("terrain transaction failed");
            return false;
    }
}

void
gentype::apply()
{
    if (this->applied) {
        return;
    }

    switch (this->transaction.state) {
        case TERRAIN_TRANSACTION_OCCUPIED:
            if (this->sorting == 0) {
                this->transaction.apply();
            } else {
                this->transaction.state = TERRAIN_TRANSACTION_APPLIED;
            }
            break;

        default:
            tms_debugf("can not generate, transaction state is not occupied");
            return;
    }

    if (this->transaction.state == TERRAIN_TRANSACTION_APPLIED) {
        this->create_entities();
        this->add_to_world();
    }

    this->applied = true;
}

void
gentype::add_to_world()
{
    if (this->transaction.state != TERRAIN_TRANSACTION_APPLIED) {
        return;
    }

    /* XXX TODO this does NOT handle groups correctly! */

    for (std::map<uint32_t, entity*>::iterator i = this->entities.begin(); i!= this->entities.end();) {
        entity *e = i->second;
        e->on_load(false, false);

        terrain_coord c(e->_pos.x, e->_pos.y);
        level_chunk *chunk = W->cwindow->get_chunk(c.chunk_x, c.chunk_y);

        if (chunk->load_phase >= 2 && chunk->body) {
            /* immediately add the entitity to the world */
            W->add(e);
            i++;
            continue;
        }

        e->pre_write();
        of::write(&W->cwindow->preloader.heap, W->level.version, e, 0, b2Vec2(0.f, 0.f), false);
        e->post_write();
        preload_info info(e->write_ptr, e->write_size, true);

        W->cwindow->preloader.entities.insert(std::make_pair(e->id, info));
        W->cwindow->preloader.entities_by_chunk.insert(std::make_pair(chunk_pos(c.chunk_x, c.chunk_y), e->id));

        /* unload the entity for later loading */
        this->entities.erase(i++);
        delete e;
    }

    W->init_level_entities(&this->entities, &this->groups);
    G->add_entities(&entities, &this->groups, &this->connections, &this->cables);
}

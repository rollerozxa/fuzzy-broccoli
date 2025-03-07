#include "misc.hh"
#include "world.hh"
#include "terrain.hh"
#include "chunk.hh"
#include "gentype.hh"

bool operator <(const terrain_coord& lhs, const terrain_coord &rhs)
{
    if (lhs.chunk_x != rhs.chunk_x) {
        return lhs.chunk_x < rhs.chunk_x;
    } else {
        return lhs.chunk_y < rhs.chunk_y;
    }
}

/**
 * Terrain generation overview
 *
 * Phase 1:
 * heightmap
 *
 * Phase 2:
 * set material based on height
 *
 * Phase 3:
 * Dig out caves
 *
 * Phase 4:
 * Merge pixels
 **/

/**
 * Occupy all chunk slots
 **/
void
terrain_transaction::occupy(gentype *gt)
{
    typedef std::multimap<terrain_coord, terrain_edit>::iterator iterator;
    iterator i, ii;

    this->state = TERRAIN_TRANSACTION_OCCUPYING;

    for (i = this->modifications.begin(); i != this->modifications.end(); ) {
        terrain_coord c = (*i).first;
        std::pair<iterator, iterator> range = this->modifications.equal_range(c);

        level_chunk *chunk = W->cwindow->get_chunk(c.chunk_x, c.chunk_y);

        tms_debugf("occupy gt %p %d %d", gt, chunk->pos_x, chunk->pos_y);

        if (this->state != TERRAIN_TRANSACTION_OCCUPYING) {
            /* another transaction invalidate this one */
            tms_debugf("INVALIDATED!!!");
            return;
        }

        for (ii = range.first; ii != range.second; ++ii) {
            terrain_coord cc = (*ii).first;

            if (!chunk->occupy_pixel(cc.get_local_x(), cc.get_local_y(), gt)) {
                tms_debugf("terrain transaction failed");
                this->state = TERRAIN_TRANSACTION_INVALIDATED;
                return;
            }
        }

        /* make sure the chunk is generated to phase 3 */
        chunk->generate(W->cwindow, 3);

        if (this->state != TERRAIN_TRANSACTION_OCCUPYING) {
            return;
        }

        i = range.second;
    }

    tms_assertf(this->state == TERRAIN_TRANSACTION_OCCUPYING, "state was not occupying");
    this->state = TERRAIN_TRANSACTION_OCCUPIED;
}

void
terrain_transaction::apply()
{
#ifdef DEBUG
    tms_assertf(this->state != TERRAIN_TRANSACTION_APPLIED, "wtf?");
#endif

    tms_debugf("applying terrain transaction");

    typedef std::multimap<terrain_coord, terrain_edit>::iterator iterator;
    iterator i, ii;

    /* make sure nearby chunks are generated to phase 3 */
    for (i = this->modifications.begin(); i != this->modifications.end(); ) {
        terrain_coord c = (*i).first;
        std::pair<iterator, iterator> range = this->modifications.equal_range(c);

        level_chunk *chunk = W->cwindow->get_chunk(c.chunk_x, c.chunk_y);

        for (int y=GENTYPE_MAX_REACH_Y; y>=-GENTYPE_MAX_REACH_Y; y--) {
            for (int x=-GENTYPE_MAX_REACH_X; x<=GENTYPE_MAX_REACH_X; x++) {
                //tms_debugf("make sure %d %d is gen 3", chunk->pos_x+x, chunk->pos_y+y);
                level_chunk *c = W->cwindow->get_chunk(chunk->pos_x+x, chunk->pos_y+y, false);
                c->generate(W->cwindow, 3);
            }
        }

        i = range.second;
    }

    if (this->state != TERRAIN_TRANSACTION_OCCUPIED) {
        return;
    }

    tms_debugf("transaction apply, state is %d", this->state );
    this->state = TERRAIN_TRANSACTION_APPLIED;

    for (i = this->modifications.begin(); i != this->modifications.end(); ) {
        terrain_coord c = (*i).first;
        std::pair<iterator, iterator> range = this->modifications.equal_range(c);

        level_chunk *chunk = W->cwindow->get_chunk(c.chunk_x, c.chunk_y);

#ifdef DEBUG_SPECIFIC_CHUNK
        if (chunk->pos_x == DEBUG_CHUNK_X && chunk->pos_y == DEBUG_CHUNK_Y) {
            tms_debugf("applying gentype in chunk %d %d", DEBUG_CHUNK_X, DEBUG_CHUNK_Y);
        }
#endif

        for (ii = range.first; ii != range.second; ++ii) {
            terrain_coord cc = (*ii).first;
            terrain_edit e = (*ii).second;

            if (e.flags & TERRAIN_EDIT_TOUCH) {

            } else {
                int local_x = cc.get_local_x();
                int local_y = cc.get_local_y();
                int new_mat = e.data;

                for (int z=0; z<3; z++) {
                    if (!(e.flags & (TERRAIN_EDIT_LAYER0 << z))) {
                        continue;
                    }

                    int prev = chunk->get_pixel(local_x, local_y, z);
                    bool apply = true;

                    if (e.flags & TERRAIN_EDIT_INC) {
                        apply = (prev < new_mat);
                    } else if (e.flags & TERRAIN_EDIT_DEC) {
                        apply = (prev > new_mat);
                    }

                    if (e.flags & TERRAIN_EDIT_SOFTEN) {
                        new_mat = prev - 1;
                    } else if (e.flags & TERRAIN_EDIT_HARDEN) {
                        new_mat = prev + 1;
                    }

                    if (e.flags & TERRAIN_EDIT_SOFT) {
                        apply = (apply && !prev);
                    } else if (e.flags & TERRAIN_EDIT_NONEMPTY) {
                        apply = (apply && prev);
                    }

                    if (apply) {
                        chunk->set_pixel(local_x, local_y, z, new_mat);
                    }
                }
            }
        }

        if (chunk->generate_phase >= 5) {
            /* XXX XXX XXX */
            tms_fatalf("This should never happen, chunk xy: %d %d, dist: %d %d", chunk->pos_x, chunk->pos_y, this->start_x-chunk->pos_x, this->start_y-chunk->pos_y);

        }

        i = range.second;
    }
}

void
chunk_window::set_seed(uint64_t seed)
{
    this->seed = seed;
    tms_infof("chunk window set seed to %" PRIu64, seed);
}

float *
chunk_window::generate_heightmap(int chunk_x, bool search)
{
    if (search) {
        level_chunk *c = this->get_chunk(chunk_x, 0);
        c->generate(this, 2);

        return this->get_heights(chunk_x);
    } else {
        float *heights = (float*)malloc(16*sizeof(float));

#if 0
        tms_debugf("generate heightmap for chunk x %d", chunk_x);
#endif

        for (int x=0; x<16; x++) {
            heights[x] = -100000.f;
        }
        this->heightmap.insert(std::make_pair(chunk_x, heights));
        return heights;
    }
}

static inline bool is_surface_level(int pos_y, float *heights)
{
    return ((pos_y+1)*8.f > heights[7] && (pos_y)*8.f < heights[7]);
}

void
level_chunk::generate(chunk_window *win, int up_to_phase/*=5*/)
{

}

#include "terrain.hh"
#include "chunk.hh"

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

void
level_chunk::generate(chunk_window *win, int up_to_phase/*=5*/)
{

}

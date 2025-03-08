#include "chunk.hh"
#include "world.hh"

static size_t _conn_id = 0;

chunk_preloader::chunk_preloader()
{
    this->w_lb.cap = 8388608; /* 2^23 */
    this->w_lb.min_cap = this->w_lb.cap;
    this->w_lb.sparse_resize = true;
    this->w_lb.size = 0;
    this->w_lb.buf = (unsigned char*)malloc(this->w_lb.cap);

    this->heap.cap = 8388608; /* 2^23 */
    this->heap.min_cap = this->heap.cap;
    this->heap.sparse_resize = true;
    this->heap.size = 0;
    this->heap.buf = (unsigned char*)malloc(this->heap.cap);

    this->reset();
}

chunk_preloader::~chunk_preloader()
{
    this->reset();
}

void
chunk_preloader::reset()
{
    /* TODO: free everything */
    _conn_id = 0;

    this->w_lb.reset();
    this->heap.reset();
    this->entities_by_chunk.clear();
    this->groups.clear();
    this->entities.clear();
    this->cables.clear();
    this->chunks.clear();
    this->connections.clear();
    this->connection_rels.clear();

    this->loaded_entities.clear();
    this->loaded_groups.clear();
    this->loaded_connections.clear();
    this->loaded_cables.clear();
    this->affected_chunks.clear();

    this->clear_chunks();
}

void
chunk_preloader::clear_chunks()
{
    for (chunk_iterator it = this->active_chunks.begin();
            it != this->active_chunks.end(); ++it) {
        it->second->remove_from_world();
        delete it->second;
    }

    for (chunk_iterator it = this->wastebin.begin();
            it != this->wastebin.end(); ++it) {
        it->second->remove_from_world();
        delete it->second;
    }
    this->active_chunks.clear();
    this->wastebin.clear();
}


level_chunk *
chunk_preloader::read_chunk(preload_info i)
{
    lvlbuf bb = i.heap ? this->heap : this->w_lb;
    lvlbuf *b = &bb;
    b->rp = i.ptr;

    int pos_x = b->r_int32();
    int pos_y = b->r_int32();
    int generate_phase = b->r_uint8();
    level_chunk *c = new level_chunk(pos_x,pos_y);
    c->generate_phase = generate_phase;

#ifdef DEBUG_SPECIFIC_CHUNK
    if (c->pos_x == DEBUG_CHUNK_X && c->pos_y == DEBUG_CHUNK_Y) {
        tms_debugf("(chunk %d,%d) reading chunk, phase: %d",
                c->pos_x, c->pos_y,
                c->generate_phase);
    }
#endif

    uint8_t load_method = b->r_uint8();

    switch (load_method) {
        case CHUNK_LOAD_MERGED:
            {
                for (int m=0; m<3; m++) {
                    c->num_merged[m] = b->r_uint8();

                    if (W->level.version < LEVEL_VERSION_1_5_1) {
                        for (int x=0; x<c->num_merged[m]; x++) {
                            c->merged[m][x] = tpixel_desc();
                            b->r_buf((char*)&c->merged[m][x], sizeof(struct tpixel_desc_1_5));
                        }
                    } else {
                        b->r_buf((char*)c->merged[m], c->num_merged[m]*sizeof(struct tpixel_desc));
                    }
                }

                c->update_pixel_buffer();
                c->update_heights();
            }
            break;

        case CHUNK_LOAD_PIXELS:
            {
                /* load by pixel values */
                for (int m=0; m<3; m++) {
                    c->num_merged[m] = 0;
                }
                b->r_buf((char*)c->pixels, sizeof(uint8_t)*3*16*16);
                c->update_heights();
            }
            break;

        case CHUNK_LOAD_EMPTY:
            break;
    }

    return c;
}

void
chunk_preloader::prepare_write()
{

}

void
chunk_preloader::write_groups(lvlinfo *lvl, lvlbuf *lb)
{
    for (std::map<uint32_t, preload_info>::iterator i = this->groups.begin(); i != this->groups.end(); i++) {
        preload_info info = i->second;
        lvlbuf *r_lb = info.heap ? &this->heap : &this->w_lb;

        lb->ensure(info.size);
        lb->w_buf((const char*)r_lb->buf+info.ptr, info.size);
    }
    tms_debugf("preloader wrote %d groups", (int)this->groups.size());
}

void
chunk_preloader::write_entities(lvlinfo *lvl, lvlbuf *lb)
{
    for (std::map<uint32_t, preload_info>::iterator i = this->entities.begin(); i != this->entities.end(); i++) {
        preload_info info = i->second;
        lvlbuf *r_lb = info.heap ? &this->heap : &this->w_lb;
        lb->ensure(info.size);
        lb->w_buf((const char*)r_lb->buf+info.ptr, info.size);
    }
    tms_debugf("preloader wrote %d entities", (int)this->entities.size());
}

void
chunk_preloader::write_cables(lvlinfo *lvl, lvlbuf *lb)
{
    for (std::map<uint32_t, preload_info>::iterator i = this->cables.begin(); i != this->cables.end(); i++) {
        preload_info info = i->second;
        lvlbuf *r_lb = info.heap ? &this->heap : &this->w_lb;
        lb->ensure(info.size);
        lb->w_buf((const char*)r_lb->buf+info.ptr, info.size);
    }
    tms_debugf("preloader wrote %d cables", (int)this->cables.size());
}

void
chunk_preloader::write_connections(lvlinfo *lvl, lvlbuf *lb)
{
    for (std::map<size_t, preload_info>::iterator i = this->connections.begin(); i != this->connections.end(); i++) {
        preload_info info = i->second;
        lvlbuf *r_lb = info.heap ? &this->heap : &this->w_lb;
        lb->ensure(info.size);
        lb->w_buf((const char*)r_lb->buf+info.ptr, info.size);
    }
    tms_debugf("preloader wrote %d connections", (int)this->connections.size());
}

/**
 * Always returns a valid chunk for the given chunk coordinates.
 * Will create an empty chunk and return it if the chunk hasn't been previously loaded
 *
 * if soft is set to true, a null chunk can be returned
 **/
level_chunk*
chunk_preloader::get_chunk(int x, int y, bool soft/*=false*/, bool load/*=true*/)
{
    chunk_pos p(x,y);
    level_chunk *c;

    std::map<chunk_pos, level_chunk*>::iterator i;
    std::map<chunk_pos, preload_info>::iterator pre_i;

    if ((i = this->active_chunks.find(p)) != this->active_chunks.end()) {
        return i->second;
    }

    if (soft) {
        return 0;
    }

    if ((i = this->wastebin.find(p)) != this->wastebin.end()) {
        i->second->garbage = false;
        //this->wastebin.erase(i);
        //this->active_chunks.insert(std::make_pair(p, c));
        return i->second;
    }

    if (!load) {
        return 0;
    }

    if ((pre_i = this->chunks.find(p)) != this->chunks.end()) {
        /* chunk was found in the preloader, we load it */
        c = this->read_chunk(pre_i->second);
        this->chunks.erase(pre_i);

        c->init_chunk_neighbours();
        this->active_chunks.insert(std::make_pair(p, c));
        return c;
    }

#ifdef DEBUG_PRELOADER_SANITY
    tms_assertf(abs(x) < 1000 && abs(y) < 1000, "get_chunk() suspicious position %d %d", x, y);
#endif

    c = new level_chunk(x,y);
    c->init_chunk_neighbours();
    this->active_chunks.insert(std::make_pair(p, c));

    return c;
}

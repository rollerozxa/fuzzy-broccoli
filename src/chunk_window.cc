#include "chunk.hh"

static struct tms_mesh    *mesh_pool[MAX_CHUNKS];
static bool   initialized;

struct vertex {
    tvec3 pos;
    tvec3 nor;
    tvec2 uv;
} __attribute__ ((packed));

struct cvert {
    tvec3 p;
    tvec3 n;
    tvec2 u;
} __attribute__((packed));

static void
_init()
{

}

chunk_window::chunk_window()
{
    this->set_mesh((struct tms_mesh*)0);
    this->set_material(&m_tpixel);
    this->reset();

    this->caveview = tms_texture_alloc();
    tms_texture_set_filtering(this->caveview, GL_LINEAR);
    this->caveview->wrap = GL_CLAMP_TO_EDGE;
    tms_texture_alloc_buffer(this->caveview, CAVEVIEW_SIZE, CAVEVIEW_SIZE, 1);
    tms_texture_clear_buffer(this->caveview, 255);
}

void
chunk_window::reset()
{
    for (std::map<int, float*>::iterator i = this->heightmap.begin(); i != this->heightmap.end(); i++) {
        free(i->second);
    }
    this->heightmap.clear();

    if (this->children) {free(this->children);this->children=0;this->num_children=0;}

    this->x = 0;
    this->y = 0;
    this->w = 0;
    this->h = 0;
    this->isset = false;
}

float*
chunk_window::get_heights(int chunk_x, bool must_be_valid/*=false*/)
{
    std::map<int, float*>::iterator i = this->heightmap.find(chunk_x);

    float *heights;

    if (i == this->heightmap.end()) {
        heights = this->generate_heightmap(chunk_x, must_be_valid);
    } else {
        heights = i->second;
    }

    return heights;
}



void
chunk_window::set(b2Vec2 lower, b2Vec2 upper)
{

}

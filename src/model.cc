#include "model.hh"
#include "misc.hh"
#include "pkgman.hh"
#include "settings.hh"

#define NUM_MISC_MODELS 3
struct tms_model *model_misc[NUM_MISC_MODELS];
int cur_model = 0;

#define MODEL_CACHE_VERSION 34

static int i1o1_shift_i = 1;
static int i2o1_shift_i = 1;
static int cpad_shift_i = 1;

struct model_load_data mesh_factory::models[NUM_MODELS] = {
    {"data/models/plank1.3ds"},
    {"data/models/plank2.3ds"},
    {"data/models/plank3.3ds"},
    {"data/models/plank4.3ds"},
    {"data/models/thinplank1.3ds"},
    {"data/models/thinplank2.3ds"},
    {"data/models/thinplank3.3ds"},
    {"data/models/thinplank4.3ds"},
    {"data/models/room.3ds"},
    {"data/models/room_corner_1.3ds"},
    {"data/models/room_corner_2.3ds"},
    {"data/models/room_corner_3.3ds"},
    {"data/models/room_corner_4.3ds"},
    {"data/models/weight.3ds"},
    {"data/models/separator.3ds"},
    {"data/models/sphere.3ds"},
    {"data/models/sphere2.3ds"},
    {"data/models/sphere3.3ds"},
    {"data/models/generator.3ds"},
    {"data/models/wmotor.3ds"},
    {"data/models/flatmotor.3ds"},
    {"data/models/simplemotor.3ds"},
    {"data/models/plug.simple.3ds"},
    {"data/models/plug.simple.low.3ds"},
    {"data/models/plug.male.3ds"},
    {"data/models/plug.female.3ds"},
    {"data/models/plug.transmitter.3ds"},
    {"data/models/c_ifplug.male.3ds"},
    {"data/models/c_ifplug.female.3ds"},
    {"data/models/script.3ds"},
    {"data/models/breadboard.3ds"},
    {"data/models/motor.3ds"},
    {"data/models/wheel.3ds"},
    {"data/models/cup.3ds"},
    {"data/models/cylinder05.3ds"},
    {"data/models/cylinder1.3ds"},
    {"data/models/cylinder1.5.3ds"},
    {"data/models/cylinder2.3ds"},
    {"data/models/wallthing00.3ds"},
    {"data/models/wallthing0.3ds"},
    {"data/models/wallthing1.3ds"},
    {"data/models/wallthing2.3ds"},
    {"data/models/joint.3ds"},
    {"data/models/plate.3ds"},
    {"data/models/platejoint_damaged.3ds"},
    {"data/models/pivotjoint.3ds"},
    {"data/models/panel.small.3ds"},
    {"data/models/clip.3ds"},
    {"data/models/cclip.3ds"},
    {"data/models/landmine.3ds"},
    {"data/models/box_notex.3ds"},
    {"data/models/box_tex.3ds"},
    {"data/models/tribox_tex0.3ds"},
    {"data/models/tribox_tex1.3ds"},
    {"data/models/tribox_tex2.3ds"},
    {"data/models/tribox_tex3.3ds"},
    {"data/models/damper_0.3ds"},
    {"data/models/damper_1.3ds"},
    {"data/models/border-new.3ds"},
    {"data/models/rocket.3ds"},
    {"data/models/thruster.3ds"},
    {"data/models/bomb.3ds"},
    {"data/models/btn.3ds"},
    {"data/models/btn_switch.3ds"},
    {"data/models/debris.3ds"},
    {"data/models/box1.3ds"},
    {"data/models/box2.3ds"},
    {"data/models/rubberend.3ds"},
    {"data/models/i1o2.3ds"},

};

static char cache_path[512];
static bool use_cache = false;

static bool
open_cache(lvlbuf *lb)
{
    FILE *fp = fopen(cache_path, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        lb->reset();
        lb->size = 0;
        lb->ensure((int)size);

        fread(lb->buf, 1, size, fp);

        fclose(fp);

        lb->size = size;

        return true;
    }

    tms_errorf("Unable to open cache_path %s", cache_path);
    return false;
}

static bool
read_cache(lvlbuf *lb)
{
    uint8_t version = lb->r_uint8();
    uint32_t num_meshes = lb->r_uint32();
    uint32_t num_models = lb->r_uint32();

    if (version != MODEL_CACHE_VERSION) {
        tms_errorf("Mismatching version code in model cache.");
        return false;
    }

    if (num_meshes != NUM_MODELS) {
        tms_errorf("Mismatching mesh count in model cache.");
        return false;
    }

    if (num_models != NUM_MISC_MODELS) {
        tms_errorf("Mismatching model count in model cache.");
        return false;
    }

    char *data;
    for (int x=0; x<num_models; ++x) {
        struct tms_model *m = (struct tms_model*)calloc(1, sizeof(struct tms_model));

        uint32_t vertices_size = lb->r_uint32();
        data = (char*)malloc(vertices_size);
        lb->r_buf(data, vertices_size);
        m->vertices = tms_gbuffer_alloc_fill(data, vertices_size);
        free(data);

        uint32_t indices_size = lb->r_uint32();
        data = (char*)malloc(indices_size);
        lb->r_buf(data, indices_size);
        m->indices = tms_gbuffer_alloc_fill(data, indices_size);
        free(data);

        m->va = tms_varray_alloc(3);
        tms_varray_map_attribute(m->va, "position", 3, GL_FLOAT, m->vertices);
        tms_varray_map_attribute(m->va, "normal", 3, GL_FLOAT, m->vertices);
        tms_varray_map_attribute(m->va, "texcoord", 2, GL_FLOAT, m->vertices);

        uint32_t num_meshes = lb->r_uint32();

        for (int n=0; n<num_meshes; ++n) {
            struct tms_mesh *mesh = tms_model_create_mesh(m);

            mesh->id = lb->r_int32();

            mesh->i_start = lb->r_int32();
            mesh->i_count = lb->r_int32();

            mesh->v_start = lb->r_int32();
            mesh->v_count = lb->r_int32();

            mesh_factory::models[mesh->id].mesh = mesh;
        }

        m->flat = 0;

        model_misc[x] = m;
    }

    return true;
}

static bool
write_cache(lvlbuf *lb)
{
    lb->w_s_uint8(MODEL_CACHE_VERSION);
    lb->w_s_uint32(NUM_MODELS);
    lb->w_s_uint32(NUM_MISC_MODELS);

    for (int x=0; x<NUM_MISC_MODELS; ++x) {
        struct tms_model *m = model_misc[x];

        lb->w_s_uint32(m->vertices->size);
        lb->w_s_buf(m->vertices->buf, m->vertices->size);

        lb->w_s_uint32(m->indices->size);
        lb->w_s_buf(m->indices->buf, m->indices->size);

        lb->w_s_uint32(m->num_meshes);

        struct tms_mesh *mesh;
        for (int n=0; n<m->num_meshes; ++n) {
            mesh = m->meshes[n];

            if (!mesh) return false;

            lb->w_s_int32(mesh->id);

            lb->w_s_int32(mesh->i_start);
            lb->w_s_int32(mesh->i_count);
            lb->w_s_int32(mesh->v_start);
            lb->w_s_int32(mesh->v_count);
        }
    }

    return true;
}

static bool
save_cache(lvlbuf *lb)
{
    FILE *fp = fopen(cache_path, "wb");
    if (fp) {
        fwrite(lb->buf, 1, lb->size, fp);
        fclose(fp);

        return true;
    }

    return false;
}

void
mesh_factory::init_models(void)
{
    snprintf(cache_path, 511, "%s/models.cache", tbackend_get_storage_path());

    GLuint err = glGetError();
    if (err != 0) {
        tms_errorf("GL Error after initializing models: %d", err);
    }

    use_cache = false;

    tms_infof("Model cache path: %s", cache_path);

    if (!settings["always_reload_data"]->v.b && file_exists(cache_path)) {
        tms_infof("Checking if we want to use cache...");
        /* The cache file exists, make sure we want to use it. */
        use_cache = true;
#ifndef TMS_BACKEND_ANDROID
        time_t cache_mtime = get_mtime(cache_path);
        time_t model_mtime;

        for (int x=0; x<NUM_MODELS; ++x) {
            if (!mesh_factory::models[x].path) continue;

            model_mtime = get_mtime(mesh_factory::models[x].path);
            if (model_mtime >= cache_mtime) {
                tms_infof("Not using cache, %s has been modified", mesh_factory::models[x].path);
                use_cache = false;
                break;
            }
        }
#endif
    }

    if (use_cache) {
        tms_infof("Initializing models... (cache)");

        lvlbuf lb;

        if (!open_cache(&lb)) {
            tms_errorf("Error opening cache, reverting to non-cached model loading.");
            use_cache = false;
        } else {
            if (read_cache(&lb)) {
                cur_mesh = NUM_MODELS;
            } else {
                tms_errorf("Error reading cache, reverting to non-cached model loading.");
                use_cache = false;
            }
        }

        if (lb.buf) {
            free(lb.buf);
        }
    }

    if (!use_cache) {
        tms_infof("Initializing models... (no cache)");
        for (int x=0; x<NUM_MISC_MODELS; ++x) {
            model_misc[x] = tms_model_alloc();
        }
    }
}

void
mesh_factory::upload_models(void)
{
    tms_infof("Uploading models...");

    for (int x=0; x<NUM_MISC_MODELS; ++x) {
        tms_model_upload(model_misc[x]);
    }

    GLuint err = glGetError();
    if (err != 0) {
        tms_errorf("GL Error after uploading models: %d", err);
    }

    if (!use_cache) {
        /* write cache file! */
        lvlbuf lb;

        /* dump models to cache file */
        if (!write_cache(&lb)) {
            tms_errorf("An error occured while trying write model cache.");
        } else {
            if (!save_cache(&lb)) {
                tms_errorf("An error occured while trying to save model cache to a file. (not enough permission/disk space?)");
            }
        }
    }
}

int cur_mesh = 0; /* extern */

/* Returns true if there are any more models to load */
bool
mesh_factory::load_next(void)
{
    if (cur_mesh >= NUM_MODELS) return false;

    struct model_load_data *mld = &mesh_factory::models[cur_mesh ++];
    struct tms_model *model = model_misc[cur_model];

    int status = T_OK;

    if (mld->path) {
        mld->mesh = tms_model_load(model, mld->path, &status);
    } else {
        /* If the base mesh model and the current chosen model are not the same,
         * we will have to reload */
        if (model != models[mld->base_id].model) {
            cur_mesh = mld->base_id;
            tms_warnf("Base mesh and shift-mesh model mismatch, reloading from base_mesh ID. (%d)", cur_mesh);

            return true;
        }

        mld->mesh = tms_model_shift_mesh_uv(models[mld->base_id].model, models[mld->base_id].mesh, mld->offset.x, mld->offset.y);
    }
    mld->mesh->id = cur_mesh - 1;
    mld->model = model;

    if (model->vertices->size > 2000000) {
        /* We exceeded 2 mil vertices with this mesh, load it again into another model */
        ++ cur_model;
        -- cur_mesh;
    }

    if (status != T_OK) {
        tms_errorf("Error loading mesh %s into model: %d", mld->path, status);
    }

    return (cur_mesh < NUM_MODELS);
}

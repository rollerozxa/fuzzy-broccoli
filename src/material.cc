#include "material.hh"
#include "settings.hh"
#include "ui.hh"
#include "simplebg.hh"
#include "gui.hh"
#include "misc.hh"

#include <locale.h>

#ifndef GLSL
#define GLSL(...) #__VA_ARGS__
#endif

#define SN(x) x->name = const_cast<char*>(#x)

struct tms_program *menu_bg_program;
GLuint              menu_bg_color_loc;
tms::shader *shader_colored;
tms::shader *shader_breadboard;
tms::shader *shader_pv_colored;
tms::shader *shader_pv_rgba;
tms::shader *shader_rubberband;
tms::shader *shader_pv_colored_m;
tms::shader *shader_edev;
tms::shader *shader_edev_m;
tms::shader *shader_edev_dark;
tms::shader *shader_edev_dark_m;
tms::shader *shader_red;
tms::shader *shader_white;
tms::shader *shader_blue;
tms::shader *shader_red_m;
tms::shader *shader_white_m;
tms::shader *shader_blue_m;
tms::shader *shader_grass;
tms::shader *shader_pv_textured;
tms::shader *shader_pv_textured_ao;
tms::shader *shader_textured_ao;
tms::shader *shader_pv_textured_ao_tinted;
tms::shader *shader_pv_textured_m;
tms::shader *shader_shiny;
tms::shader *shader_textured;
tms::shader *shader_gi;
tms::shader *shader_gi_col;
tms::shader *shader_gi_tex;
tms::shader *shader_ao;
tms::shader *shader_ao_norot;
tms::shader *shader_ao_clear;
tms::shader *shader_ao_bias;
tms::shader *shader_spritebuf;
tms::shader *shader_spritebuf_light;
tms::shader *shader_charbuf;
tms::shader *shader_charbuf2;
tms::shader *shader_linebuf;
tms::shader *shader_colorbuf;
tms::shader *shader_cable;
tms::shader *shader_wheel;
tms::shader *shader_field;
tms::shader *shader_interactive;
tms::shader *shader_interactive_m;

tms::shader *shader_border;
tms::shader *shader_border_ao;
tms::shader *shader_bg;
tms::shader *shader_bg_fixed;
tms::shader *shader_bg_colored;

tms::shader *shader_grid;

int material_factory::background_id = 0;

m m_colored;
m m_gem;
m m_cavemask;
m m_cable;
m m_pixel;
m m_iomisc;
m m_pv_colored;
m m_interactive;
m m_pv_rgba;
m m_wood;
m m_tpixel;
m m_weight;
m m_metal;
m m_iron;
m m_rail;
m m_rocket;
m m_plastic;
m m_pellet;
m m_bullet;
m m_gen;
m m_battery;
m m_motor;
m m_wmotor;
m m_gear;
m m_gear_ao;
m m_rackhouse;
m m_breadboard;
m m_room;
m m_wheel;
m m_cup;
m m_edev;
m m_edev_dark;
m m_spikes;
m m_red;
m m_heavyedev;
m m_cable_red;
m m_cable_black;
m m_cable_blue;
m m_conn;
m m_conn_no_ao;
m m_charbuf;
m m_charbuf2;
m m_spritebuf;
m m_spritebuf2;
m m_linebuf;
m m_linebuf2;
m m_bigpanel;
m m_mpanel;
m m_smallpanel;
m m_misc;
m m_border;
m m_bg;
m m_bg2;
m m_bg_fixed;
m m_grid;
m m_bg_colored;
m m_rubber;
m m_bedrock;
m m_rubberband;
m m_item;
m m_item_shiny;
m m_chest;
m m_stone;

static tms::texture *tex_wood = 0;
static tms::texture *tex_animal = 0;
static tms::texture *tex_tpixel = 0;
static tms::texture *tex_grass = 0;
static tms::texture *tex_rubber = 0;
static tms::texture *tex_reflection = 0;
static tms::texture *tex_iomisc = 0;
static tms::texture *tex_motor = 0;
static tms::texture *tex_i2o1 = 0;
static tms::texture *tex_i1o1 = 0;
static tms::texture *tex_bigpanel = 0;
static tms::texture *tex_mpanel = 0;
static tms::texture *tex_smallpanel = 0;
static tms::texture *tex_rackhouse = 0;
static tms::texture *tex_rack = 0;
static tms::texture *tex_metal = 0;
static tms::texture *tex_gen = 0;
static tms::texture *tex_battery = 0;
static tms::texture *tex_wmotor = 0;
static tms::texture *tex_breadboard = 0;
static tms::texture *tex_wheel = 0;
static tms::texture *tex_cup_ao = 0;
static tms::texture *tex_misc = 0;
static tms::texture *tex_border = 0;
static tms::texture *tex_grid = 0;

static tms::texture *tex_sprites = 0;
static tms::texture *tex_line = 0;

tms::texture *tex_bg = 0;
tms::texture *tex_bedrock = 0;

const char *available_bgs[] = {
    "Wood 1",
};
const int num_bgs = sizeof(available_bgs)/sizeof(void*);

static const char *menu_bgsources[] = {
    "attribute vec2 position;"
    "attribute vec2 texcoord;"
    "varying lowp vec2 FS_texcoord;"

    "void main(void) {"
        "FS_texcoord = texcoord * SCALE;"
        "gl_Position = vec4(position, 0, 1.);"
    "}",

    "uniform sampler2D tex_0;"
    "uniform vec4      color;"
    "varying lowp vec2 FS_texcoord;"

    "void main(void) {"
        "gl_FragColor = texture2D(tex_0, FS_texcoord)*color;"
    "}"
};

static void
read_shader(struct shader_load_data *sld, GLenum type, uint32_t global_flags, char **out)
{
    if (!(global_flags & GF_ENABLE_GI) && (sld->flags & SL_REQUIRE_GI)) {
        *out = 0;
        return;
    }

    char path[1024];

    snprintf(path, 1023, "data/shaders/%s.%s",
            sld->name, type == GL_VERTEX_SHADER ? "vp" : "fp");

    FILE_IN_ASSET(1);

    _FILE *fh = _fopen(path, "rb");
    if (fh) {
        _fseek(fh, 0, SEEK_END);
        long size = _ftell(fh);
        _fseek(fh, 0, SEEK_SET);

        *out = (char*)malloc(size+1);

        _fread(*out, 1, size, fh);

        (*out)[size] = '\0';

        _fclose(fh);
    } else {
        *out = 0;
        tms_errorf("Error reading shader at %s!", path);
    }
}

struct shader_load_data shaders[] = {
    { SL_SHARED, "border_ao",               &shader_border_ao },
    { SL_SHARED, "border",                  &shader_border },
    { SL_SHARED, "colored",                 &shader_colored },
    { SL_SHARED, "cable",                   &shader_cable },
    { SL_SHARED, "colorbuf",                &shader_colorbuf }, // TODO: We can precompute the shaded color too
    { SL_SHARED, "pv_colored",              &shader_pv_colored },
    { SL_SHARED, "rubberband",              &shader_rubberband },
    { SL_SHARED, "breadboard",              &shader_breadboard },
    { SL_SHARED, "pv_textured",             &shader_pv_textured },
    { SL_SHARED, "grass",                   &shader_grass },
    { SL_SHARED, "wheel",                   &shader_wheel },
    { SL_SHARED, "gi",                      &shader_gi },
    {
        SL_SHARED | SL_REQUIRE_GI,
        "gi_tex",
        &shader_gi_tex,
        &shader_gi
    },
    {
        SL_SHARED | SL_REQUIRE_GI,
        "gi_col",
        &shader_gi_col,
        &shader_gi
    },
    { SL_SHARED, "ao",                      &shader_ao },
    { SL_SHARED, "ao_norot",                &shader_ao_norot },
    { SL_SHARED, "ao_clear",                &shader_ao_clear },
    { SL_SHARED, "ao_bias",                 &shader_ao_bias },
    { SL_SHARED, "pv_textured_ao",          &shader_pv_textured_ao },
    { SL_SHARED, "textured_ao",             &shader_textured_ao },
    { SL_SHARED, "pv_textured_ao_tinted",   &shader_pv_textured_ao_tinted },
    { SL_SHARED, "textured",                &shader_textured },
    { SL_SHARED, "linebuf",                 &shader_linebuf },
    { SL_SHARED, "spritebuf",               &shader_spritebuf },
    { SL_SHARED, "spritebuf_light",         &shader_spritebuf_light },
    { SL_SHARED, "charbuf",                 &shader_charbuf },
    { SL_SHARED, "charbuf2",                &shader_charbuf2 },
    { SL_SHARED, "grid",                    &shader_grid },
    { SL_SHARED, "bg_fixed",                &shader_bg_fixed },
    { SL_SHARED, "bg_colored",              &shader_bg_colored },
    { SL_SHARED, "bg",                      &shader_bg },
    { SL_SHARED, "shiny",                   &shader_shiny },
    { SL_SHARED, "colored_field",           &shader_field },

    /* menu shaders */
    { SL_SHARED, "pv_colored_m",            &shader_pv_colored_m },
    { SL_SHARED, "pv_textured_m",           &shader_pv_textured_m },
};

static int num_shaders = sizeof(shaders) / sizeof(shaders[0]);

static const char *src_constcolored_m[] = {
GLSL(
    attribute vec3 position;
    attribute vec3 normal;
    attribute vec2 texcoord;

    varying mediump float FS_diffuse;
    varying mediump vec2 FS_texcoord;

    uniform mat4 MVP;

    void main(void)
    {
        FS_diffuse = clamp(dot(vec3(0,0,1), normal), 0., .8)
            /*+ min(position.z-1., 0.)*.5*/;
        gl_Position = MVP*vec4(position, 1.);
    }
),
GLSL(
    varying mediump float FS_diffuse;

    void main(void)
    {
        float ambient =  AMBIENT_M;
        vec4 color = COLOR;
        gl_FragColor = vec4(color.rgb * FS_diffuse + color.rgb * ambient, color.a);
    }
)
};

static const char *src_constcolored[] = {
GLSL(
    attribute vec3 position;
    attribute vec3 normal;

    uniform mat4 MVP;
    uniform mat3 N;
    UNIFORMS

    varying lowp vec2 FS_diffuse;
    VARYINGS

    void main(void)
    {
        vec3 nor = N*normal;

        vec4 pos = MVP*vec4(position, 1.);
        SET_SHADOW
        SET_AMBIENT_OCCL
        SET_GI
        FS_diffuse = vec2(clamp(dot(LIGHT, nor)*_DIFFUSE, 0., 1.), .05*nor.z);
        gl_Position = pos;
    }
),
GLSL(
    UNIFORMS

    varying lowp vec2 FS_diffuse;
    VARYINGS

    GI_FUN

    void main(void)
    {
        gl_FragColor = SHADOW * COLOR * FS_diffuse.x
        + COLOR * (_AMBIENT + FS_diffuse.y) * AMBIENT_OCCL GI;
    }
)
};
/*
static const char *src_wheel[] = {
GLSL(
    attribute vec3 position;
    attribute vec3 normal;
    attribute vec2 texcoord;

    varying lowp vec2 FS_diffuse;
    varying lowp vec2 FS_texcoord;
    VARYINGS

    uniform mat4 MVP;
    uniform mat4 MV;
    uniform mat3 N;
    UNIFORMS

    varying lowp vec3 FS_normal;
    varying lowp vec3 FS_eye;

    void main(void)
    {
        vec3 nor = N*normal;
        vec4 pos = MVP*vec4(position, 1.);

        SET_SHADOW
        SET_AMBIENT_OCCL

        FS_texcoord = texcoord;
        FS_diffuse = vec2(clamp(dot(LIGHT, nor)*_DIFFUSE, 0., 1.), .05*nor.z);

        FS_normal = nor;
        FS_eye = (MV*vec4(position, 1.)).xyz;

        gl_Position = pos;
    }
),
GLSL(
    uniform sampler2D tex_0;
    UNIFORMS

    varying lowp vec2 FS_diffuse;
    varying lowp vec2 FS_texcoord;
    varying lowp vec3 FS_normal;
    varying lowp vec3 FS_eye;
    VARYINGS

    void main(void)
    {
        vec4 color = texture2D(tex_0, FS_texcoord);
        vec3 n = normalize(FS_normal);
        vec3 e = normalize(FS_eye);
        vec3 R = normalize(reflect(LIGHT, n));
        float specular = pow(clamp(dot(R, e), .0, 1.), 6.);
        gl_FragColor = SHADOW * (color + color*specular) * FS_diffuse.x + color.a * color * (_AMBIENT + FS_diffuse.y)*AMBIENT_OCCL
                        ;
    }
)
};
*/

void
material_factory::upload_all()
{
    //tex_wood->upload();
    //tex_metal->upload();
}

void
material_factory::free_shaders()
{
    tms_infof("Freeing shaders...");
    int ierr;
    //tms_assertf((ierr = glGetError()) == 0, "gl error %d at beginning of free shaders", ierr);

    tms_infof("FREEING SHADER_EDEV: %p", shader_edev);
    delete shader_edev;
    delete shader_edev_dark;
    delete shader_edev_m;
    delete shader_edev_dark_m;
    delete shader_bg;
    delete shader_grid;
    delete shader_bg_fixed;
    delete shader_bg_colored;
    delete shader_field;
    delete shader_border;
    delete shader_border_ao;
    delete shader_breadboard;
    delete shader_pv_colored;
    delete shader_rubberband;
    delete shader_pv_colored_m;
    delete shader_pv_textured;
    delete shader_wheel;
    delete shader_grass;
    delete shader_pv_textured_ao;
    delete shader_textured_ao;
    delete shader_pv_textured_ao_tinted;
    delete shader_pv_textured_m;
    delete shader_shiny;
    delete shader_textured;
    if (shader_gi_tex == shader_gi) {
        shader_gi_tex = 0;
    } else {
        delete shader_gi_tex;
    }
    if (shader_gi_col == shader_gi) {
        shader_gi_col = 0;
    } else {
        delete shader_gi_col;
    }
    delete shader_gi;
    delete shader_ao;
    delete shader_ao_norot;
    delete shader_ao_clear;
    delete shader_ao_bias;
    delete shader_spritebuf;
    delete shader_spritebuf_light;
    delete shader_charbuf;
    delete shader_charbuf2;
    delete shader_linebuf;
    delete shader_cable;
    delete shader_colorbuf;

    tms_infof("Done freeing shaders...");

    //tms_assertf((ierr = glGetError()) == 0, "gl error %d at end of free shaders", ierr);
}
static int last_loaded = -1;

void
material_factory::load_bg_texture(bool soft)
{
    tms_debugf("Load BG Texture...");
    char bgname[256];

    if (material_factory::background_id >= num_bgs || material_factory::background_id < 0)
        material_factory::background_id = 0;

    if (!tex_bg) tex_bg = new tms::texture();
    else {
        if (soft && last_loaded == material_factory::background_id) {
            return;
        }
    }
    last_loaded = material_factory::background_id;


    sprintf(bgname, "data/bg/%d.jpg", material_factory::background_id);

    if (tex_bg->load(bgname) != T_OK)
        tex_bg->load("data/bg/0.jpg");

    tex_bg->format = GL_RGB;

    tex_bg->wrap = GL_REPEAT;
    tms_texture_set_filtering(tex_bg, TMS_MIPMAP);
    tex_bg->gamma_correction = settings["gamma_correct"]->v.b;


    tex_bg->upload();
    tms_texture_free_buffer(tex_bg);

    tms_debugf("Done");
}

#define TEX_LAZYLOAD_FN(name, body) \
    static void lz_##name(struct tms_texture *_tex_##name)\
    { tms::texture *tex_##name = static_cast<tms::texture*>(_tex_##name); \
      body }

TEX_LAZYLOAD_FN(tpixel,
    tms_texture_load(tex_tpixel,"data/textures/tpixel.jpg");
    tex_tpixel->format = GL_RGB;
    tex_tpixel->gamma_correction = settings["gamma_correct"]->v.b;
    tms_texture_set_filtering(tex_tpixel, GL_LINEAR);
    tms_texture_upload(tex_tpixel);
    tms_texture_free_buffer(tex_tpixel);
)

TEX_LAZYLOAD_FN(grass,
    tms_texture_load(tex_grass,"data/textures/grass.png");
    tex_grass->format = GL_RGBA;
    tex_grass->gamma_correction = settings["gamma_correct"]->v.b;
    tms_texture_set_filtering(tex_grass, GL_LINEAR);
    tms_texture_upload(tex_grass);
    tms_texture_free_buffer(tex_grass);
)

TEX_LAZYLOAD_FN(grid,
    tms_texture_load(tex_grid,"data/textures/grid.png");
    tex_grid->format = GL_RGBA;
    tex_grid->gamma_correction = settings["gamma_correct"]->v.b;
    tms_texture_set_filtering(tex_grid, TMS_MIPMAP);
    tms_texture_upload(tex_grid);
    tms_texture_free_buffer(tex_grid);
)

TEX_LAZYLOAD_FN(wood,
    tex_wood->load("data/textures/wood.jpg");
    tex_wood->format = GL_RGB;
    tms_texture_set_filtering(tex_wood, TMS_MIPMAP);
    tex_wood->gamma_correction = settings["gamma_correct"]->v.b;
    tex_wood->upload();
    tms_texture_free_buffer(tex_wood);
)


TEX_LAZYLOAD_FN(rubber,
    tex_rubber->load("data/textures/rubber.jpg");
    tex_rubber->format = GL_RGB;
    tms_texture_set_filtering(tex_rubber, TMS_MIPMAP);
    tex_rubber->gamma_correction = settings["gamma_correct"]->v.b;
    tex_rubber->upload();
    tms_texture_free_buffer(tex_rubber);
)

TEX_LAZYLOAD_FN(reflection,
    tex_reflection->load("data/textures/reflection.jpg");
    tex_reflection->format = GL_RGB;
    tms_texture_set_filtering(tex_reflection, GL_LINEAR);
    tex_reflection->gamma_correction = settings["gamma_correct"]->v.b;
    tex_reflection->upload();
    tms_texture_free_buffer(tex_reflection);
)


TEX_LAZYLOAD_FN(iomisc,
    tex_iomisc->gamma_correction = settings["gamma_correct"]->v.b;
    tex_iomisc->format = GL_RGBA;
    tex_iomisc->load("data/textures/iomisc.png");
    tex_iomisc->upload();
    tms_texture_free_buffer(tex_iomisc);
)

TEX_LAZYLOAD_FN(gen,
    tex_gen->gamma_correction = settings["gamma_correct"]->v.b;
    tex_gen->format = GL_RGBA;
    tex_gen->load("data/textures/generator.png");
    tms_texture_set_filtering(tex_gen, TMS_MIPMAP);
    tex_gen->upload();
    tms_texture_free_buffer(tex_gen);
)

TEX_LAZYLOAD_FN(motor,
    tex_motor->gamma_correction = settings["gamma_correct"]->v.b;
    tex_motor->format = GL_RGBA;
    tex_motor->load("data/textures/motor.png");
    tex_motor->upload();
    tms_texture_free_buffer(tex_motor);
)

TEX_LAZYLOAD_FN(misc,
    tex_misc->gamma_correction = settings["gamma_correct"]->v.b;
    tex_misc->format = GL_RGBA;
    tex_misc->load("data/textures/misc.png");
    //tms_texture_set_filtering(tex_misc, TMS_MIPMAP);
    tex_misc->upload();
    tms_texture_free_buffer(tex_misc);
)

TEX_LAZYLOAD_FN(wmotor,
    tex_wmotor->gamma_correction = settings["gamma_correct"]->v.b;
    tex_wmotor->format = GL_RGB;
    tex_wmotor->load("data/textures/wmotor.png");
    tex_wmotor->upload();
    tms_texture_free_buffer(tex_wmotor);
)

TEX_LAZYLOAD_FN(metal,
    tex_metal->load("data/textures/metal.jpg");
    tex_metal->format = GL_RGB;
    tms_texture_set_filtering(tex_metal, TMS_MIPMAP);
    tex_metal->gamma_correction = settings["gamma_correct"]->v.b;
    tex_metal->upload();
    tms_texture_free_buffer(tex_metal);
)

TEX_LAZYLOAD_FN(i1o1,
    tex_i1o1->gamma_correction = settings["gamma_correct"]->v.b;
    tex_i1o1->format = GL_RGBA;
    tex_i1o1->load("data/textures/i1o1.png");
    tms_texture_set_filtering(tex_i1o1, TMS_MIPMAP);
    tex_i1o1->upload();
    //tex_i1o1->set_filtering(TMS_MIPMAP);
    tms_texture_free_buffer(tex_i1o1);
)

TEX_LAZYLOAD_FN(smallpanel,
    tex_smallpanel->gamma_correction = settings["gamma_correct"]->v.b;
    tex_smallpanel->format = GL_RGBA;
    tex_smallpanel->load("data/textures/smallpanel.png");
    tex_smallpanel->upload();
    tms_texture_free_buffer(tex_smallpanel);
)

TEX_LAZYLOAD_FN(rack,
    tex_rack->gamma_correction = settings["gamma_correct"]->v.b;
    tex_rack->format = GL_RGBA;
    tex_rack->load("data/textures/rack.png");
    tex_rack->upload();
    tms_texture_free_buffer(tex_rack);
)

TEX_LAZYLOAD_FN(wheel,
    tex_wheel->gamma_correction = settings["gamma_correct"]->v.b;
    tex_wheel->format = GL_RGBA;
    tex_wheel->load("data/textures/wheel.png");
    tex_wheel->upload();
    tms_texture_free_buffer(tex_wheel);
)

TEX_LAZYLOAD_FN(breadboard,
    tex_breadboard->gamma_correction = settings["gamma_correct"]->v.b;
    tex_breadboard->format = GL_RGB;
    tex_breadboard->load("data/textures/breadboard.jpg");
    tex_breadboard->upload();
    tms_texture_free_buffer(tex_breadboard);
)

TEX_LAZYLOAD_FN(cup_ao,
    tex_cup_ao->gamma_correction = settings["gamma_correct"]->v.b;
    tex_cup_ao->format = GL_RGBA;
    tex_cup_ao->load("data/textures/cup_ao.png");
    tex_cup_ao->upload();
    tms_texture_free_buffer(tex_cup_ao);
)

TEX_LAZYLOAD_FN(border,
    tex_border->load("data/textures/border.jpg");
    tex_border->format = GL_RGB;
    tms_texture_set_filtering(tex_border, TMS_MIPMAP);
    tex_border->gamma_correction = settings["gamma_correct"]->v.b;
    tex_border->upload();
    tms_texture_free_buffer(tex_border);
)

TEX_LAZYLOAD_FN(sprites,
    tex_sprites->gamma_correction = settings["gamma_correct"]->v.b;
    tex_sprites->format = GL_RGBA;
    tex_sprites->load("data/textures/sprites.png");
    tex_sprites->upload();
    tms_texture_free_buffer(tex_sprites);
)

TEX_LAZYLOAD_FN(line,
    tex_line->gamma_correction = settings["gamma_correct"]->v.b;
    tex_line->format = GL_RGBA;
    tex_line->load("data/textures/line.png");
    tex_line->upload();
    tms_texture_free_buffer(tex_line);
)


void
material_factory::init()
{
    setlocale(LC_ALL, "C");
    setlocale(LC_NUMERIC, "C");

    material_factory::background_id = 0;
    int ierr;
    bool enable_gi = false; // settings["enable_gi"]->v.b

    /* XXX: a gl error occurs when a gtk dialog is shown */
    tms_assertf((ierr = glGetError()) == 0, "gl error %d at material factory init", ierr);

    tms_infof("Initializing material factor...");

    material_factory::init_shaders();

    /* TEXTURES BEGIN */
    tms_infof("Initializing textures... ");

#define TEX_INIT_LAZYLOAD(x) {tex_##x = new tms::texture(); tms_texture_set_buffer_fn(tex_##x, lz_##x);}
    TEX_INIT_LAZYLOAD(tpixel);
    TEX_INIT_LAZYLOAD(grass);
    TEX_INIT_LAZYLOAD(grid);
    TEX_INIT_LAZYLOAD(wood);
    TEX_INIT_LAZYLOAD(rubber);
    TEX_INIT_LAZYLOAD(reflection);
    TEX_INIT_LAZYLOAD(iomisc);
    TEX_INIT_LAZYLOAD(gen);
    TEX_INIT_LAZYLOAD(motor);
    TEX_INIT_LAZYLOAD(misc);
    TEX_INIT_LAZYLOAD(wmotor);
    TEX_INIT_LAZYLOAD(metal);
    TEX_INIT_LAZYLOAD(smallpanel);
    TEX_INIT_LAZYLOAD(rack);
    TEX_INIT_LAZYLOAD(wheel);
    TEX_INIT_LAZYLOAD(breadboard);
    TEX_INIT_LAZYLOAD(cup_ao);
    TEX_INIT_LAZYLOAD(border);
    TEX_INIT_LAZYLOAD(sprites);
    TEX_INIT_LAZYLOAD(line);
#undef TEX_INIT_LAZYLOAD

    material_factory::load_bg_texture();

    /* TEXTURES END */

    material_factory::init_materials();
}

void
material_factory::init_shaders()
{
    setlocale(LC_ALL, "C");
    setlocale(LC_NUMERIC, "C");

    tms_infof("Defining shader globals...");

    bool dynamic_lighting = false;
    _tms.gamma_correct = (int)settings["gamma_correct"]->v.b;

    /* Default ambient/diffuse values */
    if (_tms.gamma_correct) {
        //this->default_ambient = .1f;
        //this->default_diffuse = .95f;
        P.default_ambient = .225f;
        P.default_diffuse = 2.8f;
    } else {
        P.default_ambient = .55f;
        P.default_diffuse = 1.1f;
    }

    int ierr;
    char tmp[512];
    bool enable_gi = false; // settings["enable_gi"]->v.b

    tms_shader_global_clear_defines();

#ifndef TMS_BACKEND_ANDROID
    tms_shader_global_define_vs("lowp", "");
    tms_shader_global_define_fs("lowp", "");
    tms_shader_global_define_vs("mediump", "");
    tms_shader_global_define_fs("mediump", "");
    tms_shader_global_define_vs("highp", "");
    tms_shader_global_define_fs("highp", "");
#endif

    if (settings["shadow_map_precision"]->v.i == 0 && !settings["shadow_map_depth_texture"]->is_true()) {
        tms_shader_global_define("SHADOW_BIAS", ".15");
    } else {
        tms_shader_global_define("SHADOW_BIAS", ".005");
    }

    tvec3 light = P.get_light_normal();
    sprintf(tmp, "vec3(%f,%f,%f)", light.x, light.y, light.z);
    tms_shader_global_define("LIGHT", tmp);

    if (settings["enable_shadows"]->v.b) {
        switch (settings["shadow_quality"]->v.u8) {
            default: case 0: case 2:
                tms_shader_global_define_vs("SET_SHADOW",
                        "vec4 shadow = SMVP*pos;"
                        "FS_shadow_z = shadow.z;"
                        "FS_shadow = shadow.xy;");
                break;
            case 1:
            {
                sprintf(tmp,
                        "vec4 shadow = SMVP*pos; FS_shadow_z = shadow.z; FS_shadow = shadow.xy; FS_shadow_dither = shadow.xy + vec2(%f, %f);",
                        //1.f / settings["shadow_map_resx"]->v.i,
                        //1.f / settings["shadow_map_resy"]->v.i
                        1.f / _tms.window_width,
                        1.f / _tms.window_height
                        );
                tms_shader_global_define_vs("SET_SHADOW", tmp);
                break;
            }
        }
    } else {
        tms_shader_global_define_vs("SET_SHADOW", "");
    }

    if (settings["enable_ao"]->v.b) {
        if (!settings["shadow_ao_combine"]->v.b) {
            tms_shader_global_define_vs("SET_AMBIENT_OCCL", "FS_ao = (AOMVP * pos).xy;");
            tms_shader_global_define_vs("SET_AMBIENT_OCCL2", "FS_ao = (AOMVP * pos).xy;");
        } else {
            tms_shader_global_define_vs("SET_AMBIENT_OCCL", "FS_ao = (SMVP * (pos - position.z*MVP[2])).xy;");
            tms_shader_global_define_vs("SET_AMBIENT_OCCL2", "FS_ao = (SMVP * (pos - (position.z+1.0)*MVP[2])).xy;");
        }
    } else {
        tms_shader_global_define_vs("SET_AMBIENT_OCCL", "");
        tms_shader_global_define_vs("SET_AMBIENT_OCCL2", "");
    }

    if (enable_gi) {
        tms_shader_global_define_vs("SET_GI", "FS_gi = (SMVP * (pos + vec4(0.,-2.25,0.,0.))).xy;");
        tms_shader_global_define("ENABLE_GI","1");
        tms_shader_global_define_fs("GI","+ get_gi()");
    } else {
        tms_shader_global_define_vs("SET_GI", "");
        tms_shader_global_define_fs("GI","");
    }

    if (settings["enable_shadows"]->v.b || settings["enable_ao"]->v.b) {
        if (settings["shadow_ao_combine"]->v.b) {
            if (dynamic_lighting) {
                tms_debugf("dl=1 ");
                tms_shader_global_define_vs("UNIFORMS", "uniform mat4 SMVP;uniform lowp vec2 _AMBIENTDIFFUSE;");
            } else {
                tms_debugf("dl=0 ");
                tms_shader_global_define_vs("UNIFORMS", "uniform mat4 SMVP;");
            }
        } else {
            char tmp[1024];
            tmp[0]='\0';

            if (settings["enable_shadows"]->v.b) {
                strcat(tmp, "uniform mat4 SMVP;");
            }
            if (settings["enable_ao"]->v.b) {
                strcat(tmp, "uniform mat4 AOMVP;");
            }
            if (dynamic_lighting) {
                strcat(tmp, "uniform lowp vec2 _AMBIENTDIFFUSE;");
            }

            tms_shader_global_define_vs("UNIFORMS", tmp);
        }
    } else {
        tms_debugf("sao=0 ");
        if (dynamic_lighting) {
            tms_debugf("dl=1 ");
            tms_shader_global_define_vs("UNIFORMS", "uniform lowp vec2 _AMBIENTDIFFUSE;");
        } else {
            tms_shader_global_define_vs("UNIFORMS", "");
        }
    }

    sprintf(tmp, "%s%s%s%s",
            settings["enable_ao"]->v.b ? "uniform lowp vec3 ao_mask;" : "",
            settings["enable_shadows"]->v.b ? "uniform lowp sampler2D tex_3;" : "",
            settings["enable_ao"]->v.b ? "uniform lowp sampler2D tex_4;" : "",
            dynamic_lighting ? "uniform lowp vec2 _AMBIENTDIFFUSE;" : ""
            );
    tms_shader_global_define_fs("UNIFORMS", tmp);

    tms_shader_global_define_fs("GI_FUN",
            enable_gi ?
            "vec4 get_gi(){"
            "vec4 r = vec4(0.,0.,0.,0.);"
            "vec2 offs[16];"
            "float xx = .004;"
            "offs[0] = vec2(0.0, 0.0);"
            "offs[1] = vec2(xx, 0.0);"
            "offs[2] = vec2(xx, xx);"
            "offs[3] = vec2(0., xx);"
            "offs[4] = vec2(-xx, xx);"
            "offs[5] = vec2(-xx, 0.);"
            "offs[6] = vec2(-xx, -xx);"
            "offs[7] = vec2(0., -xx);"
            "offs[8] = vec2(0.0, 0.0);"
            "offs[9] = vec2(xx*2., 0.0);"
            "offs[10] = vec2(xx*2., xx*2.);"
            "offs[11] = vec2(0., xx*2.);"
            "offs[12] = vec2(-xx*2., xx*2.);"
            "offs[13] = vec2(-xx*2., 0.);"
            "offs[14] = vec2(-xx*2., -xx*2.);"
            "offs[15] = vec2(0., -xx*2.);"
            "for (int x=0; x<16; x++){"
            "vec4 s = texture2D(tex_3, FS_gi + offs[x]);"
            "float d = FS_shadow_z-.1 - s.g;"
            "r+=vec4(s.rba*.2, 0.)*float(d > 0. && d < .2);"
            "}return clamp(r / 16., 0, .2);}" : "");

    sprintf(tmp, "%s%s%s%s%s",
            settings["enable_shadows"]->v.b ? "varying lowp float FS_shadow_z;" : "",
            settings["enable_shadows"]->v.b ? "varying lowp vec2 FS_shadow;" : "",
            settings["shadow_quality"]->v.u8 == 1 ? "varying lowp vec2 FS_shadow_dither;" : "",
            settings["enable_ao"]->v.b ? "varying lowp vec2 FS_ao;" : "",
            enable_gi ? "varying lowp vec2 FS_gi;" : ""
            );
    tms_shader_global_define("VARYINGS", tmp);

#define COOL_THING ".005"

    if (settings["enable_shadows"]->v.b)
        switch (settings["shadow_quality"]->v.u8) {
            default: case 0:
                if (settings["shadow_map_depth_texture"]->is_true()) {
                    tms_shader_global_define_fs("SHADOW", "float(texture2D(tex_3, FS_shadow).g > FS_shadow_z- " COOL_THING ")");
                } else {
                    tms_shader_global_define_fs("SHADOW", "float(texture2D(tex_3, FS_shadow).g > FS_shadow_z)");
                }
                break;
            case 1:
                if (settings["shadow_map_depth_texture"]->is_true()) {
                    tms_shader_global_define_fs("SHADOW", "((float(texture2D(tex_3, FS_shadow).g > FS_shadow_z-" COOL_THING ") + float(texture2D(tex_3, FS_shadow_dither).g > FS_shadow_z-" COOL_THING "))*.5)");
                } else {
                    tms_shader_global_define_fs("SHADOW", "((float(texture2D(tex_3, FS_shadow).g > FS_shadow_z) + float(texture2D(tex_3, FS_shadow_dither).g > FS_shadow_z))*.5)");
                }
                break;

            case 2:
                tms_shader_global_define_fs("SHADOW", "(1. - dot(vec3(lessThan(texture2D(tex_3, FS_shadow).rgb, vec3(FS_shadow_z))), vec3(.3333333,.3333333,.3333333)))");
                break;
        }
    else
        tms_shader_global_define_fs("SHADOW", "1.0");

    if (settings["enable_shadows"]->v.b) {
        if (settings["gamma_correct"]->v.b)
            tms_shader_global_define_fs("AMBIENT_OCCL_FACTOR", ".9");
        else
            tms_shader_global_define_fs("AMBIENT_OCCL_FACTOR", ".5");
    } else /* boost AO factor if shadows off */
        tms_shader_global_define_fs("AMBIENT_OCCL_FACTOR", ".7");

    if (settings["enable_ao"]->v.b) {
        tms_shader_global_define_fs("AMBIENT_OCCL", "(1. - AMBIENT_OCCL_FACTOR*dot(texture2D(tex_4, FS_ao).xyz, ao_mask))");
        tms_shader_global_define_fs("AMBIENT_OCCL2", "(1. - AMBIENT_OCCL_FACTOR*dot(texture2D(tex_4, FS_ao).xyz, ao_mask2.xyz))");
        tms_shader_global_define("ENABLE_AO", "1");
    } else {
        tms_shader_global_define_fs("AMBIENT_OCCL", "1.0");
        tms_shader_global_define_fs("AMBIENT_OCCL2", "1.0");
        //tms_shader_global_define("ENABLE_AO", "0");
    }

    if (settings["shadow_ao_combine"]->v.b) {
        tms_shader_global_define("SHADOW_AO_COMBINE", "1");
    }

    if (!dynamic_lighting) {
        char _tmp[32];
        setlocale(LC_ALL, "C");
        setlocale(LC_NUMERIC, "C");
        sprintf(_tmp, "%f", P.default_ambient);
        tms_shader_global_define("_AMBIENT", _tmp);
        setlocale(LC_ALL, "C");
        setlocale(LC_NUMERIC, "C");
        sprintf(_tmp, "%f", P.default_diffuse);
        tms_shader_global_define("_DIFFUSE", _tmp);
    } else {
        tms_shader_global_define("_AMBIENT", "_AMBIENTDIFFUSE.x");
        tms_shader_global_define("_DIFFUSE", "_AMBIENTDIFFUSE.y");
    }

    tms_shader_global_define("AMBIENT_M", ".75");

    tms_material_init(static_cast<tms_material*>(&m_colored));

    tms_assertf((ierr = glGetError()) == 0, "gl error %d before shader compile", ierr);

    tms_infof("Compiling shaders");

    uint32_t global_flags = 0;

    if (enable_gi) {
        global_flags |= GF_ENABLE_GI;
    }

    for (int x=0; x<num_shaders; ++x) {
        struct shader_load_data *sld = &shaders[x];

        char *buf;
        int r;

        tms_debugf("Reading %s vertex shader...", sld->name);
        read_shader(sld, GL_VERTEX_SHADER, global_flags, &buf);
        if (!buf) {
            tms_infof("Falling back, failed to read!");
            *sld->shader = (sld->fallback ? *sld->fallback : 0);
            continue;
        }

        tms_infof("Compiling %s vertex shader...", sld->name);
        //tms_infof("Data: '%s'", buf);
        tms::shader *sh = new tms::shader(sld->name);
        r = sh->compile(GL_VERTEX_SHADER, buf);
        free(buf);
        buf = 0;

        tms_debugf("Reading %s fragment shader...", sld->name);
        read_shader(sld, GL_FRAGMENT_SHADER, global_flags, &buf);
        if (!buf) {
            tms_infof("Falling back, failed to read!");
            *sld->shader = (sld->fallback ? *sld->fallback : 0);
            continue;
        }

        tms_infof("Compiling %s fragment shader...", sld->name);
        //tms_infof("Data: '%s'", buf);
        r = sh->compile(GL_FRAGMENT_SHADER, buf);
        free(buf);
        buf = 0;

        *sld->shader = sh;
    }

    tms::shader *sh;

    sh = new tms::shader("edev");

    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.07074,.07074,.07074,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.3,.3,.3,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_edev = sh;

    sh = new tms::shader("edev dark");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.02899,.02899,.02899,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.2,.2,.2,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_edev_dark = sh;

    sh = new tms::shader("red");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.45626,.0993,.0993,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.7,.35,.35,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_red = sh;

    sh = new tms::shader("blue");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.1726,.1726,.612065,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.45,.45,.8,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_blue = sh;

    sh = new tms::shader("white");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.612065,.612065,.612065,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.8,.8,.8,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_white = sh;

    sh = new tms::shader("interactive");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.9,0.,0.,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.8,.6,.6,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored[1]);
    shader_interactive = sh;

    /* menu shaders */

    sh = new tms::shader("edev menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.07074,.07074,.07074,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.3,.3,.3,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_edev_m = sh;

    sh = new tms::shader("edev dark menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.02899,.02899,.02899,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.2,.2,.2,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_edev_dark_m = sh;

    sh = new tms::shader("red menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.45626,.0993,.0993,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.7,.35,.35,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_red_m = sh;

    sh = new tms::shader("blue menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.1726,.1726,.612065,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.45,.45,.8,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_blue_m = sh;

    sh = new tms::shader("white menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.612065,.612065,.612065,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.8,.8,.8,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_white_m = sh;

    sh = new tms::shader("interactive menu");
    if (_tms.gamma_correct) {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.612065,.325,.325,1.)");
    } else {
        tms_shader_define((struct tms_shader*)sh, "COLOR", "vec4(.8,.6,.6,1.)");
    }
    sh->compile(GL_VERTEX_SHADER, src_constcolored_m[0]);
    sh->compile(GL_FRAGMENT_SHADER, src_constcolored_m[1]);
    shader_interactive_m = sh;

    setlocale(LC_ALL, "C");
    setlocale(LC_NUMERIC, "C");

    sh = new tms::shader("Menu BG");
    {char tmp[32];
    sprintf(tmp, "vec2(%f,%f)", _tms.window_width / 512.f, _tms.window_height / 512.f);
    tms_shader_define_vs((struct tms_shader*)sh, "SCALE", tmp);}
    sh->compile(GL_VERTEX_SHADER, menu_bgsources[0]);
    sh->compile(GL_FRAGMENT_SHADER, menu_bgsources[1]);
    menu_bg_program = sh->get_program(TMS_NO_PIPELINE);
    menu_bg_color_loc = tms_program_get_uniform(menu_bg_program, "color");

    SN(shader_shiny);
    SN(shader_edev);
    SN(shader_edev_m);
    SN(shader_edev_dark);
    SN(shader_edev_dark_m);
    SN(shader_interactive);
    SN(shader_interactive_m);
    SN(shader_red);
    SN(shader_white);
    SN(shader_blue);
    SN(shader_red_m);
    SN(shader_white_m);
    SN(shader_blue_m);

    tms_infof("Done with shaders!\n");
}

void
material_factory::init_materials()
{
    tms_infof("Initializing materials");

    _tms.gamma_correct = settings["gamma_correct"]->v.b;
    bool shadow_ao_combine = settings["shadow_ao_combine"]->v.b;
    bool enable_gi = false; // settings["enable_gi"]->v.b

    m_bg.pipeline[0].program = shader_bg->get_program(0);
    m_bg.pipeline[1].program = 0;
    m_bg.pipeline[0].texture[0] = tex_bg;

    m_bg2.pipeline[0].program = shader_bg->get_program(0);
    m_bg2.pipeline[1].program = 0;
    m_bg2.pipeline[0].texture[0] = tex_wood;

    m_bg_fixed.pipeline[0].program = shader_bg_fixed->get_program(0);
    m_bg_fixed.pipeline[1].program = 0;
    m_bg_fixed.pipeline[0].texture[0] = tex_bg;

    m_bg_colored.pipeline[0].program = shader_bg_colored->get_program(0);
    m_bg_colored.pipeline[1].program = 0;
    m_bg_colored.pipeline[2].program = 0;
    m_bg_colored.pipeline[3].program = 0;

    m_grid.pipeline[0].program = shader_grid->get_program(0);
    //m_grid.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE;
    //m_grid.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_grid.pipeline[0].blend_mode = TMS_BLENDMODE__ONE_MINUS_DST_COLOR__ONE_MINUS_SRC_ALPHA;
    m_grid.pipeline[1].program = 0;
    m_grid.pipeline[0].texture[0] = tex_grid;

    m_border.pipeline[0].program = shader_border->get_program(0);
    m_border.pipeline[1].program = shader_gi->get_program(1);
    m_border.pipeline[2].program = 0;
    if (shadow_ao_combine) {
        m_border.pipeline[3].program = shader_border_ao->get_program(3);
    } else {
        m_border.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_border.pipeline[0].texture[0] = tex_border;
    m_border.pipeline[2].texture[0] = tex_border;
    m_border.type = TYPE_WOOD2;

    m_colored.pipeline[0].program = shader_colored->get_program(0);
    m_colored.pipeline[1].program = shader_gi->get_program(1);
    m_colored.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_colored.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_colored.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_colored.friction = .6f;
    m_colored.density = .5f*M_DENSITY;
    m_colored.restitution = .3f;
    m_colored.type = TYPE_PLASTIC;

    m_rubberband.pipeline[0].program = shader_rubberband->get_program(0);
    m_rubberband.pipeline[1].program = shader_gi->get_program(1);
    m_rubberband.pipeline[2].program = 0;
    m_rubberband.pipeline[3].program = 0;

    m_cable.pipeline[0].program = shader_cable->get_program(0);

    if ((float)settings["shadow_map_resx"]->v.i / (float)_tms.window_width < .7f) {
        /* disable cable shadows if the resolution is too low, it just looks ugly */
        m_cable.pipeline[1].program = 0;
    } else {
        m_cable.pipeline[1].program = shader_gi->get_program(1);
    }

    m_cable.pipeline[2].program = shader_cable->get_program(2);
    m_cable.pipeline[3].program = 0;

    m_pixel.pipeline[0].program = shader_colorbuf->get_program(0);
    m_pixel.pipeline[1].program = shader_gi->get_program(1);
    m_pixel.pipeline[2].program = 0;
    if (shadow_ao_combine) {
        m_pixel.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_pixel.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_pixel.friction = .6f;
    m_pixel.density = .5f*M_DENSITY;
    m_pixel.restitution = .3f;
    m_pixel.type = TYPE_PLASTIC;

    m_cavemask.pipeline[0].program = shader_edev_dark->get_program(0);
    m_cavemask.pipeline[1].program = 0;
    m_cavemask.pipeline[2].program = 0;
    m_cavemask.pipeline[3].program = shader_ao_clear->get_program(3);

    m_pv_colored.pipeline[0].program = shader_pv_colored->get_program(0);
    m_pv_colored.pipeline[1].program = shader_gi_col->get_program(1);
    m_pv_colored.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_pv_colored.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_pv_colored.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_pv_colored.friction = .6f;
    m_pv_colored.density = .5f*M_DENSITY;
    m_pv_colored.restitution = .3f;
    m_pv_colored.type = TYPE_PLASTIC;

    m_interactive.pipeline[0].program = shader_interactive->get_program(0);
    m_interactive.pipeline[1].program = shader_gi->get_program(1);
    m_interactive.pipeline[2].program = shader_interactive_m->get_program(2);
    if (shadow_ao_combine) {
        m_interactive.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_interactive.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_interactive.friction = .8f;
    m_interactive.density = 1.25f*M_DENSITY;
    m_interactive.restitution = .3f;
    m_interactive.type = TYPE_PLASTIC;

    m_pv_rgba.pipeline[0].program = shader_pv_colored->get_program(0);
    m_pv_rgba.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    //m_pv_rgba.pipeline[1].program = shader_gi->get_program(1);
    m_pv_rgba.pipeline[2].program = shader_pv_colored_m->get_program(2);
    //m_pv_rgba.pipeline[3].program = shader_ao->get_program(3);
    m_pv_rgba.friction = .6f;
    m_pv_rgba.density = .5f*M_DENSITY;
    m_pv_rgba.restitution = .3f;
    m_pv_rgba.type = TYPE_PLASTIC;

    m_gen.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_gen.pipeline[1].program = shader_gi->get_program(1);
    m_gen.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_gen.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_gen);
    m_gen.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_gen);
    if (shadow_ao_combine) {
        m_gen.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_gen.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_gen.friction = .5f;
    m_gen.density = 2.0f*M_DENSITY;
    m_gen.restitution = .1f;
    m_gen.type = TYPE_METAL;

    m_motor.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_motor.pipeline[1].program = shader_gi->get_program(1);
    m_motor.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_motor.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_motor);
    m_motor.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_motor);
    if (shadow_ao_combine) {
        m_motor.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_motor.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_motor.friction = .5f;
    m_motor.density = 2.0f*M_DENSITY;
    m_motor.restitution = .1f;
    m_motor.type = TYPE_METAL;

    m_room.pipeline[0].program = shader_breadboard->get_program(0);
    m_room.pipeline[1].program = shader_gi->get_program(1);
    m_room.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_room.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_breadboard);
    m_room.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_breadboard);
    m_room.pipeline[3].program = 0;//shader_ao->get_program(3);
    m_room.friction = .9f;
    m_room.density = .025f*M_DENSITY;
    m_room.restitution = .05f;
    m_room.type = TYPE_WOOD2;

    m_breadboard.pipeline[0].program = shader_breadboard->get_program(0);
    m_breadboard.pipeline[1].program = shader_gi->get_program(1);
    m_breadboard.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_breadboard.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_breadboard);
    m_breadboard.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_breadboard);
    m_breadboard.pipeline[3].program = 0;//shader_ao->get_program(3);
    m_breadboard.friction = .7f;
    m_breadboard.density = .1f*M_DENSITY;
    m_breadboard.restitution = .4f;
    m_breadboard.type = TYPE_WOOD2;


    m_wood.pipeline[0].program = shader_pv_textured->get_program(0);
    m_wood.pipeline[1].program = shader_gi->get_program(1);
    m_wood.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_wood.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_wood);
    if (enable_gi) m_wood.pipeline[1].texture[0] = static_cast<tms_texture*>(tex_wood);
    m_wood.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_wood);
    if (shadow_ao_combine) {
        m_wood.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_wood.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_wood.friction = .6f;
    m_wood.density = .5f*M_DENSITY;
    m_wood.restitution = .3f;
    m_wood.type = TYPE_WOOD;

    m_tpixel.pipeline[0].program = shader_pv_textured->get_program(0);
    m_tpixel.pipeline[1].program = shader_gi_tex->get_program(1);
    m_tpixel.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_tpixel.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_tpixel);
    if (enable_gi) m_tpixel.pipeline[1].texture[0] = static_cast<tms_texture*>(tex_tpixel);
    m_tpixel.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_tpixel);
    if (shadow_ao_combine) {
        m_tpixel.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_tpixel.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_tpixel.friction = .6f;
    m_tpixel.density = .5f*M_DENSITY;
    m_tpixel.restitution = .3f;
    m_tpixel.type = TYPE_PLASTIC;

    m_weight.pipeline[0].program = shader_pv_colored->get_program(0);
    m_weight.pipeline[1].program = shader_gi_col->get_program(1);
    m_weight.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_weight.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_weight.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_weight.friction = .6f;
    m_weight.density = 25.0f*M_DENSITY; /* XXX: Should this density really be used? */
    m_weight.restitution = .3f;
    m_weight.type = TYPE_METAL2;

    m_rubber.pipeline[0].program = shader_pv_textured->get_program(0);
    m_rubber.pipeline[1].program = shader_gi->get_program(1);
    m_rubber.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_rubber.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_rubber);
    m_rubber.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_rubber);
    if (shadow_ao_combine) {
        m_rubber.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_rubber.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_rubber.friction = 1.75f;
    m_rubber.density = .75f*M_DENSITY;
    m_rubber.restitution = .5f;
    m_rubber.type = TYPE_RUBBER;

    m_metal.pipeline[0].program = shader_pv_textured->get_program(0);/* XXX */
    m_metal.pipeline[1].program = shader_gi->get_program(1);
    m_metal.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_metal.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_metal);
    m_metal.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_metal);
    if (shadow_ao_combine) {
        m_metal.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_metal.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_metal.friction = .2f;
    m_metal.density = 1.0f*M_DENSITY;
    m_metal.restitution = .4f;
    m_metal.type = TYPE_METAL;

    m_iron.pipeline[0].program = shader_textured->get_program(0);
    m_iron.pipeline[1].program = shader_gi->get_program(1);
    m_iron.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_iron.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_metal);
    m_iron.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_metal);
    if (shadow_ao_combine) {
        m_iron.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_iron.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_iron.friction = 0.5f;
    m_iron.density = 4.f*M_DENSITY;
    m_iron.restitution = .6f; /* TODO: previous: .9f */
    m_iron.type = TYPE_METAL;

    m_rocket.pipeline[0].program = shader_pv_textured->get_program(0);/* XXX */
    m_rocket.pipeline[1].program = shader_gi->get_program(1);
    m_rocket.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_rocket.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_metal); /* XXX */
    m_rocket.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_metal); /* XXX */
    if (shadow_ao_combine) {
        m_rocket.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_rocket.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_rocket.friction = .2f;
    m_rocket.density = 1.0f*M_DENSITY;
    m_rocket.restitution = .005f;
    m_rocket.type = TYPE_METAL;

    m_plastic.pipeline[0].program = shader_pv_colored->get_program(0);
    m_plastic.pipeline[1].program = shader_gi_col->get_program(1);
    m_plastic.pipeline[2].program = shader_pv_colored_m->get_program(2);
    if (shadow_ao_combine) {
        m_plastic.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_plastic.pipeline[3].program = shader_ao_norot->get_program(3);
    }

    m_plastic.friction = .4f;
    m_plastic.density = 1.35f*M_DENSITY;
    m_plastic.restitution = .2f;
    m_plastic.type = TYPE_PLASTIC;

    m_iomisc.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_iomisc.pipeline[1].program = shader_gi->get_program(1);
    m_iomisc.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_iomisc.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_iomisc);
    m_iomisc.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_iomisc);
    if (shadow_ao_combine) {
        m_iomisc.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_iomisc.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_iomisc.friction = m_edev.friction;
    m_iomisc.density = m_edev.density;
    m_iomisc.restitution = m_edev.restitution;
    m_iomisc.type = m_edev.type;

    m_smallpanel.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_smallpanel.pipeline[1].program = shader_gi->get_program(1);
    m_smallpanel.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_smallpanel.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_smallpanel);
    m_smallpanel.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_smallpanel);
    if (shadow_ao_combine) {
        m_smallpanel.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_smallpanel.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_smallpanel.friction = .5f;
    m_smallpanel.density = .7f*M_DENSITY;
    m_smallpanel.restitution = .1f;
    m_smallpanel.type = TYPE_PLASTIC;

    m_misc.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_misc.pipeline[1].program = shader_gi->get_program(1);
    m_misc.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_misc.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_misc);
    m_misc.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_misc);
    if (shadow_ao_combine) {
        m_misc.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_misc.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_misc.friction = .5f;
    m_misc.density = .7f*M_DENSITY;
    m_misc.restitution = .3f;
    m_misc.type = TYPE_PLASTIC;

    /* TODO: use src_constcolored */
    m_conn.pipeline[0].program = shader_edev->get_program(0);
    m_conn.pipeline[1].program = shader_gi->get_program(1);
    m_conn.pipeline[2].program = 0;
    m_conn.pipeline[3].program = shader_ao_bias->get_program(3);

    m_conn_no_ao.pipeline[0].program = shader_edev->get_program(0);
    m_conn_no_ao.pipeline[1].program = shader_gi->get_program(1);
    m_conn_no_ao.pipeline[2].program = 0;
    m_conn_no_ao.pipeline[3].program = 0;

    m_red.pipeline[0].program = shader_red->get_program(0);
    m_red.pipeline[1].program = shader_gi->get_program(1);
    m_red.pipeline[2].program = shader_red_m->get_program(2);
    if (shadow_ao_combine) {
        m_red.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_red.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_red.friction = .5f;
    m_red.density = .5f*M_DENSITY;
    m_red.restitution = .5f;
    m_red.type = TYPE_PLASTIC;

    m_cable_red.pipeline[0].program = shader_red->get_program(0);
    m_cable_red.pipeline[1].program = shader_gi->get_program(1);
    m_cable_red.pipeline[2].program = shader_red_m->get_program(2);

    m_cable_black.pipeline[0].program = shader_white->get_program(0);
    m_cable_black.pipeline[1].program = shader_gi->get_program(1);
    m_cable_black.pipeline[2].program = shader_white_m->get_program(2);

    m_cable_blue.pipeline[0].program = shader_blue->get_program(0);
    m_cable_blue.pipeline[1].program = shader_gi->get_program(1);
    m_cable_blue.pipeline[2].program = shader_blue_m->get_program(2);

    m_heavyedev.pipeline[0].program = shader_edev->get_program(0);
    m_heavyedev.pipeline[1].program = shader_gi->get_program(1);
    m_heavyedev.pipeline[2].program = shader_edev_m->get_program(2);
    if (shadow_ao_combine) {
        m_heavyedev.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_heavyedev.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_heavyedev.friction = .2f;
    m_heavyedev.density = 1.0f*M_DENSITY;
    m_heavyedev.restitution = .4f;
    m_heavyedev.type = TYPE_PLASTIC;

    m_edev.pipeline[0].program = shader_edev->get_program(0);
    m_edev.pipeline[1].program = shader_gi->get_program(1);
    m_edev.pipeline[2].program = shader_edev_m->get_program(2);
    if (shadow_ao_combine) {
        m_edev.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_edev.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_edev.friction = .5f;
    m_edev.density = .5f*M_DENSITY;
    m_edev.restitution = .2f;
    m_edev.type = TYPE_PLASTIC;

    m_edev_dark.pipeline[0].program = shader_edev_dark->get_program(0);
    m_edev_dark.pipeline[1].program = shader_gi->get_program(1);
    m_edev_dark.pipeline[2].program = shader_edev_dark_m->get_program(2);
    if (shadow_ao_combine) {
        m_edev_dark.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_edev_dark.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_edev_dark.friction = .5f;
    m_edev_dark.density = .5f*M_DENSITY;
    m_edev_dark.restitution = .2f;
    m_edev_dark.type = TYPE_PLASTIC;

    m_wheel.pipeline[0].program = shader_wheel->get_program(0);
    m_wheel.pipeline[1].program = shader_gi->get_program(1);
    m_wheel.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_wheel.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_wheel);
    m_wheel.pipeline[0].texture[1] = static_cast<tms_texture*>(tex_reflection);
    m_wheel.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_wheel);
    if (shadow_ao_combine) {
        m_wheel.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_wheel.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_wheel.friction = 1.5f;
    m_wheel.density = .5f*M_DENSITY;
    m_wheel.restitution = .4f;
    m_wheel.type = TYPE_RUBBER;

    m_wmotor.pipeline[0].program = shader_pv_textured->get_program(0);
    m_wmotor.pipeline[1].program = shader_gi->get_program(1);
    m_wmotor.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_wmotor.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_wmotor);
    m_wmotor.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_wmotor);
    if (shadow_ao_combine) {
        m_wmotor.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_wmotor.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_wmotor.type = TYPE_METAL;

    m_cup.pipeline[0].program = shader_pv_textured_ao->get_program(0);
    m_cup.pipeline[1].program = shader_gi->get_program(1);
    m_cup.pipeline[2].program = shader_pv_textured_m->get_program(2);
    m_cup.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_cup_ao);
    m_cup.pipeline[2].texture[0] = static_cast<tms_texture*>(tex_cup_ao);
    if (shadow_ao_combine) {
        m_cup.pipeline[3].program = shader_ao->get_program(3);
    } else {
        m_cup.pipeline[3].program = shader_ao_norot->get_program(3);
    }
    m_cup.type = TYPE_PLASTIC;

    m_linebuf.pipeline[0].program = shader_linebuf->get_program(0);
    m_linebuf.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_linebuf.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_line);
    m_linebuf.pipeline[1].program = 0;
    m_linebuf.pipeline[2].program = 0;

    m_linebuf2.pipeline[0].program = shader_linebuf->get_program(0);
    m_linebuf2.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE;
    m_linebuf2.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_line);
    m_linebuf2.pipeline[1].program = 0;
    m_linebuf2.pipeline[2].program = 0;

    m_spritebuf.pipeline[0].program = shader_spritebuf->get_program(0);
    m_spritebuf.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_spritebuf.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_sprites);
    m_spritebuf.pipeline[1].program = 0;
    m_spritebuf.pipeline[2].program = 0;

    m_spritebuf2.pipeline[0].program = shader_spritebuf_light->get_program(0);
    m_spritebuf2.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE;
    m_spritebuf2.pipeline[0].texture[0] = static_cast<tms_texture*>(tex_sprites);
    m_spritebuf2.pipeline[1].program = 0;
    m_spritebuf2.pipeline[2].program = 0;

    m_charbuf.pipeline[0].program = shader_charbuf->get_program(0);
    //m_charbuf.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_charbuf.pipeline[0].blend_mode = TMS_BLENDMODE_OFF;
    m_charbuf.pipeline[0].texture[0] = &gui_spritesheet::atlas_text->texture;
    m_charbuf.pipeline[1].program = 0;
    m_charbuf.pipeline[2].program = 0;

    m_charbuf2.pipeline[0].program = shader_charbuf2->get_program(0);
    m_charbuf2.pipeline[0].blend_mode = TMS_BLENDMODE__SRC_ALPHA__ONE_MINUS_SRC_ALPHA;
    m_charbuf2.pipeline[0].texture[0] = &gui_spritesheet::atlas_text->texture;
    m_charbuf2.pipeline[1].program = 0;
    m_charbuf2.pipeline[2].program = 0;

}

#pragma once

#include <tms/bindings/cpp/cpp.hh>

extern int cur_mesh;

enum {
    MODEL_THINPLANK1,
    MODEL_THINPLANK2,
    MODEL_THINPLANK3,
    MODEL_THINPLANK4,
    MODEL_WEIGHT,
    MODEL_SEPARATOR,
    MODEL_SPHERE,
    MODEL_SPHERE2,
    MODEL_SPHERE3,
    MODEL_GENERATOR,
    MODEL_WMOTOR,
    MODEL_SIMPLEMOTOR,
    MODEL_PLUG_SIMPLE,
    MODEL_PLUG_SIMPLE_LOW,
    MODEL_PLUG_MALE,
    MODEL_PLUG_FEMALE,
    MODEL_PLUG_TRANSMITTER,
    MODEL_IFPLUG_MALE,
    MODEL_IFPLUG_FEMALE,
    MODEL_BREADBOARD,
    MODEL_DMOTOR,
    MODEL_WHEEL,
    MODEL_CYLINDER05,
    MODEL_CYLINDER1,
    MODEL_CYLINDER15,
    MODEL_CYLINDER2,
    MODEL_WALLTHING00,
    MODEL_WALLTHING0,
    MODEL_WALLTHING1,
    MODEL_WALLTHING2,
    MODEL_WELDJOINT,
    MODEL_PLATEJOINT,
    MODEL_PLATEJOINT_DAMAGED,
    MODEL_PIVOTJOINT,
    MODEL_PANEL_SMALL,
    MODEL_CLIP,
    MODEL_CCLIP,
    MODEL_LANDMINE,
    MODEL_DAMPER_0,
    MODEL_DAMPER_1,
    MODEL_ROCKET,
    MODEL_BUTTON,
    MODEL_BUTTON_SWITCH,
    MODEL_DEBRIS,
    MODEL_RUBBEREND,
    MODEL_I1O2,

    NUM_MODELS
};

struct model_load_data {
    const char *path;
    int base_id;
    tvec2 offset;
    struct tms_mesh *mesh;
    struct tms_model *model;
};

class mesh_factory
{
  public:
    static struct model_load_data models[NUM_MODELS];

    static inline struct tms_mesh *get_mesh(int model)
    {
        return mesh_factory::models[model].mesh;
    }

    static void init_models(void);
    static bool load_next(void);
    static void upload_models(void);
};
